#include <iostream>

#include <minizinc/minizinc_eval.hh>

int main(void) {

  std::string model = R"(
  
  function tuple(array[int] of int, array[int] of float):
    main(array[int] of int: x, array[int] of float: y) = ([2*xi | xi in x],y);
  
)";

  MiniZinc::MznEvaluator e;
  
  e.loadModelString(model);
  
  auto ret = e.eval({{1,2,3},{4.0,5.0,6.0}});

  std::cout << "result:\n";
  for (const auto& x : ret.first) {
    std::cerr << x << ", ";
  }
  std::cerr << "\n";
  for (const auto& x : ret.second) {
    std::cerr << x << ", ";
  }
  std::cerr << "\n";

  ret = e.eval({{13,21,34},{4.20,5.20,6.20}});

  std::cout << "result:\n";
  for (const auto& x : ret.first) {
    std::cerr << x << ", ";
  }
  std::cerr << "\n";
  for (const auto& x : ret.second) {
    std::cerr << x << ", ";
  }
  std::cerr << "\n";

}
