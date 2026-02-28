#include "chuffed/globals/blackbox.h"
#include "chuffed/core/propagator.h"
#include "chuffed/vars/vars.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <iostream>

#ifndef _WIN32
#include <cstdlib>
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#endif

BlackBoxDLL::BlackBoxDLL(const std::string &name) {
  std::string loadError;
#ifdef _WIN32
  library = LoadLibraryA(name.c_str());
  if (!library) {
    loadError = std::string("unable to locate library `") + name + "'";
    library = LoadLibraryA((std::string(name) + ".dll").c_str());
  }
  if (!library) {
    library = LoadLibraryA((std::string("lib") + name + ".dll").c_str());
  }
#else
  library = dlopen(name.c_str(), RTLD_LAZY);
  if (!library) {
    loadError = std::string(dlerror());
    library = dlopen((name + ".so").c_str(), RTLD_NOW);
  }
  if (!library) {
    library = dlopen((std::string("lib") + name + ".so").c_str(), RTLD_NOW);
  }
#endif
  if (!library) {
    throw std::string("BlackboxDLL: Unable to open dynamic library: " + loadError);
  }

  // find symbol for blacbox function
#ifdef _WIN32
  *(void **)(&dll_fzn_blackbox) =
      GetProcAddress((HMODULE)library, "fzn_blackbox");
  std::string symError = ".";
#else
  *(void **)(&dll_fzn_blackbox) = dlsym(library, "fzn_blackbox");
  std::string symError(": ");
  if (!dll_fzn_blackbox) {
    symError += std::string(dlerror());
  }
#endif
  if (!dll_fzn_blackbox) {
    throw std::string("BlackboxDLL: Unable to find symbol `fzn_blackbox` in dynamic library" +
                    symError);
  }
}

BlackBoxDLL::~BlackBoxDLL() {
  if (library) {
#ifdef _WIN32
    FreeLibrary((HMODULE)library);
#else
    dlclose(library);
#endif
  }
}

BlackBoxExec::BlackBoxExec(const std::string &program) {
#ifdef _WIN32
  SECURITY_ATTRIBUTES saAttr;
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  HANDLE g_hChildStd_IN_Rd = NULL;
  HANDLE g_hChildStd_IN_Wr = NULL;
  HANDLE g_hChildStd_OUT_Rd = NULL;
  HANDLE g_hChildStd_OUT_Wr = NULL;

  // Create a pipe for the child process's STDOUT.
  if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
    std::cerr << "Stdout CreatePipe" << std::endl;
  // Ensure the read handle to the pipe for STDOUT is not inherited.
  if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
    std::cerr << "Stdout SetHandleInformation" << std::endl;

  // Create a pipe for the child process's STDIN
  if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
    std::cerr << "Stdin CreatePipe" << std::endl;
  // Ensure the write handle to the pipe for STDIN is not inherited.
  if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
    std::cerr << "Stdin SetHandleInformation" << std::endl;

  PROCESS_INFORMATION piProcInfo;
  STARTUPINFOA siStartInfo;

  // Set up members of the PROCESS_INFORMATION structure.
  ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

  // Set up members of the STARTUPINFO structure.
  // This structure specifies the STDIN and STDOUT handles for redirection.
  ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
  siStartInfo.cb = sizeof(STARTUPINFOA);
  siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
  siStartInfo.hStdInput = g_hChildStd_IN_Rd;
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  std::string prog = program;
  BOOL processStarted =
      CreateProcessA(nullptr,
                     &prog[0],  // command line
                     nullptr,      // process security attributes
                     nullptr,      // primary thread security attributes
                     TRUE,         // handles are inherited
                     0,            // creation flags
                     nullptr,      // use parent's environment
                     nullptr,      // use parent's current directory
                     &siStartInfo, // STARTUPINFO pointer
                     &piProcInfo); // receives PROCESS_INFORMATION

  if (!processStarted) {
    throw std::string("BlackBoxExec: Unable to start program `" + program + "'");
  }

  CloseHandle(piProcInfo.hThread);
  // Stop ReadFile from blocking
  CloseHandle(g_hChildStd_OUT_Wr);
  // Just close the child's in pipe here
  CloseHandle(g_hChildStd_IN_Rd);

  pipe_send = g_hChildStd_IN_Wr;
  pipe_receive = g_hChildStd_OUT_Rd;
#else
  const int READ = 0;
  const int WRITE = 1;
  int child_in[2];
  int child_out[2];
  pipe(child_in);
  pipe(child_out);

  if (int childPID = fork()) {
    close(child_in[READ]);
    close(child_out[WRITE]);

    pipe_send = child_in[WRITE];
    int pipe_receive = child_out[READ];
    file_receive = fdopen(pipe_receive, "r");
    return;
  }
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  dup2(child_in[READ], STDIN_FILENO);
  dup2(child_out[WRITE], STDOUT_FILENO);
  close(child_in[WRITE]);
  close(child_out[READ]);

  int status = std::system(program.c_str());
  std::exit(status);
#endif
};

BlackBoxExec::~BlackBoxExec() {
#ifdef _WIN32
  CloseHandle(pipe_send);
  CloseHandle(pipe_receive);
#else
  close(pipe_send);
  fclose(file_receive);
#endif
}

void BlackBoxExec::run(const std::vector<int> &int_in,
                       const std::vector<double> &float_in,
                       std::vector<int> &int_out,
                       std::vector<double> &float_out) {
  // Construct program input
  std::stringstream out;
  for (int i : int_in) {
    out << i << " ";
  }
  out << "; ";
  for (int f : float_in) {
    out << f << " ";
  }
  out << ";\n";
  std::string out_buf = out.str();
#ifdef _WIN32
  // Write to process input pipe
  BOOL success =
      WriteFile(pipe_send, out_buf.c_str(), out_buf.size(), nullptr, nullptr);
  assert(success);

  // Read output from process by pipe
  char c[2] = {0, 0};
  std::ostringstream oss;
  while (c[0] != '\n') {
    DWORD count = 0;
    BOOL success = ReadFile(pipe_receive, c, sizeof(c) - 1, &count, NULL);
    if (!success) {
      throw std::string(
          "BlackBoxExec: Reading blackbox process output from pipe resulted did not succeed");
    } else if (count == 0) {
      throw std::string("BlackBoxExec: Blackbox process provided an incomplete response");
    }
    assert(count == 1);
    oss << c[0];
  }
  std::string in_buffer(oss.str());
#else
  // Write to process input pipe
  ssize_t bytes_written = write(pipe_send, out_buf.c_str(), out_buf.size());
  assert(bytes_written = out_buf.size());

  // Read from process output pipe
  char *str = NULL;
  size_t size = 0;

  if (getline(&str, &size, file_receive) == -1) {
    throw std::string(
        "BlackBoxExec: Reading blackbox process output from pipe resulted in error no. " +
            std::to_string(errno));
  }
  std::string in_buffer(str);
  free(str);
#endif
  // Parse given line
  std::istringstream ss(std::move(in_buffer));
  for (size_t i = 0; i < int_out.size(); ++i) {
    ss >> int_out[i];
    if (ss.fail()) {
      throw std::string("BlackBoxExec: Failed to read output integer " +
                                      std::to_string(i) +
                                      " from blackbox process output, " +
                                      std::to_string(int_out.size()) +
                                      " integer values where expected.");
    }
  }
  char E;
  ss >> E;
  assert(E == ';');
  for (size_t i = 0; i < float_out.size(); ++i) {
    ss >> float_out[i];
    if (ss.fail()) {
      throw std::string("BlackBoxExec: Failed to read output float " +
                                      std::to_string(i) +
                                      " from blackbox process output, " +
                                      std::to_string(float_out.size()) +
                                      " floating point values where expected.");
    }
  }
  ss >> E;
  assert(E == ';');
  std::string rem(ss.str().substr(ss.tellg()));
  assert(std::all_of(rem.begin(), rem.end(), [](char c) {
    return c == '\n' || c == ' ' || c == '\t' || c == '\r';
  }));
}


template <int U = 0, int V = 0>
class BlackBox : public Propagator {
public:
	const int sz_in;
	const int sz_out;
	IntView<U>* int_input;
	IntView<V>* int_output;
	BlackBoxFn* bb_fn;

	BlackBox(vec<IntView<U>> _int_input, vec<IntView<V>> _int_output, BlackBoxFn* _bb_fn) : sz_in(_int_input.size()), sz_out(_int_output.size()), int_input(_int_input.release()), int_output(_int_output.release()), bb_fn(_bb_fn) {
		priority = 5;
		for (int i = 0; i < sz_in; ++i) {
			int_input[i].attach(this, i, EVENT_F);
		}
	}

	bool propagate() override {
		// std::cerr << "Black Box Fn input: ";
		std::vector<int> int_in(sz_in);
		for (int i = 0; i < sz_in; ++i) {
			if (!int_input[i].isFixed()) {
				// std::cerr << "(cancelled: not all fixed)\n";
				return true;
			}
			// std::cerr << int_input[i].val() << " ";
			int_in[i] = int_input[i].getVal();
		}
		std::vector<int> int_out(sz_out);

		std::vector<double> float_in(0);
    std::vector<double> float_out(0);

    bb_fn->run(int_in, float_in, int_out, float_out);

    // std::cerr << "Black Box Fn output: ";
    Clause* reason = nullptr;
    for (int i = 0; i < sz_out; i++) {
      // std::cerr << int_out[i] << " ";
      if (int_output[i].setValNotR(int_out[i])) {
      	if (reason == nullptr) {
      		reason = Reason_new(sz_in + 1);
	     		for (int j = 0; j < sz_in; ++j) {
			      (*reason)[j+1] = int_input[j].getValLit();
	       	}
       	}

				if (!int_output[i].setVal(int_out[i], reason)) {
					return false;
				}
			}
    }
    // std::cerr << std::endl;

		return true;
	}
};


template <int U = 0, int V = 0>
class BlackBoxBounds : public Propagator {
public:
	const int sz;
	IntView<U>* x;
	std::vector<std::vector<std::pair<int, BB_Bounds>>> reason;
	BlackBoxFn* bb_fn;

	BlackBoxBounds(vec<IntView<U>> _x, std::vector<std::vector<std::pair<int, BB_Bounds>>> _reason, BlackBoxFn* _bb_fn) : sz(_x.size()), x(_x.release()), reason(_reason), bb_fn(_bb_fn) {
		priority = 5;
		for (int i = 0; i < sz; ++i) {
			x[i].attach(this, i, EVENT_LU);
		}
	}

	Clause* create_reason(int i, BB_Bounds b) {
		const std::vector<std::pair<int, BB_Bounds>>& my_reason = reason[i*2 + (b-1)];
		Clause* reason = Reason_new(my_reason.size() + 1);
		for (int j = 0; j < my_reason.size(); ++j) {
			const std::pair<int, BB_Bounds>& term = my_reason[j];
			(*reason)[j+1] = term.second == BB_LB ? x[term.first].getMinLit() : x[term.first].getMaxLit();
		}
		return reason;
	}

	bool propagate() override {
		// std::cerr << "Black Box Fn input: ";
		std::vector<int> bounds_in(sz*2);
		for (int i = 0; i < sz; ++i) {
			// std::cerr << x[i].getMin() << " " << x[i].getMax() << " ";
			bounds_in[i*2] = x[i].getMin();
			bounds_in[i*2+1] = x[i].getMax();
		}
		// std::cerr << std::endl;
		std::vector<int> bounds_out(sz*2);

		std::vector<double> float_in(0);
    std::vector<double> float_out(0);

    bb_fn->run(bounds_in, float_in, bounds_out, float_out);

    // std::cerr << "Black Box Fn output: ";
    for (int i = 0; i < sz; i++) {
      // std::cerr << bounds_out[i*2] << " " << bounds_out[i*2+1] << " ";
      if (x[i].setMinNotR(bounds_out[i*2])) {
				if (!x[i].setMin(bounds_out[i*2], create_reason(i, BB_LB))) {
					return false;
				}
			}
      if (x[i].setMaxNotR(bounds_out[i*2+1])) {
				if (!x[i].setMax(bounds_out[i*2+1], create_reason(i, BB_UB))) {
					return false;
				}
			}
    }
    // std::cerr << std::endl;

		return true;
	}
};


void blackbox(vec<IntVar*>& int_in, vec<IntVar*>& int_out, const std::string &mode, const std::string &instantiation) {
	BlackBoxFn *black_box(nullptr);
	if (mode == "dll") {
    black_box = new BlackBoxDLL(instantiation);
  } else if (mode == "exec") {
    black_box = new BlackBoxExec(instantiation);
  } else {
    throw std::string("blackbox: unknown blackbox protocol `" + mode + "'");
  }

	vec<IntView<> > in_view;
	for (unsigned int i = 0; i < int_in.size(); i++) {
		in_view.push(IntView<>(int_in[i]));
	}

	vec<IntView<> > out_view;
	for (unsigned int i = 0; i < int_out.size(); i++) {
		out_view.push(IntView<>(int_out[i]));
	}

	new BlackBox<>(in_view, out_view, black_box);
}

void blackbox_bounds(vec<IntVar*>& ivar, std::vector<std::vector<std::pair<int, BB_Bounds>>> reason, const std::string &mode, const std::string &instantiation) {

	BlackBoxFn *black_box(nullptr);
	if (mode == "dll") {
    black_box = new BlackBoxDLL(instantiation);
  } else if (mode == "exec") {
    black_box = new BlackBoxExec(instantiation);
  } else {
    throw std::string("blackbox: unknown blackbox protocol `" + mode + "'");
  }

	vec<IntView<> > views;
	for (unsigned int i = 0; i < ivar.size(); i++) {
		views.push(IntView<>(ivar[i]));
	}

	new BlackBoxBounds<>(views, reason, black_box);
}
