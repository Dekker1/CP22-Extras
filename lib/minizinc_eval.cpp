/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */

/*
 *  Main authors:
 *     Guido Tack <guido.tack@monash.edu>
 */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <strstream>

#include <minizinc/exception.hh>
#include <minizinc/minizinc_eval.hh>
#include <minizinc/parser.hh>
#include <minizinc/typecheck.hh>
#include <minizinc/builtins.hh>
#include <minizinc/flatten_internal.hh>
#include <minizinc/eval_par.hh>

namespace MiniZinc {

MznEvaluator::MznEvaluator(std::string stdlibDir) :_stdlibDir(stdlibDir), _env(nullptr) {}

void MznEvaluator::load(const std::vector<std::string> filenames,
                        const std::string& textModel, const std::string& textModelName) {
  std::vector<std::string> includePaths {_stdlibDir + "/std/"};

  for (auto& includePath : includePaths) {
    if (!FileUtils::directory_exists(includePath)) {
      std::ostringstream oss;
      oss << "cannot access include directory " << includePath << "\n";
      throw MiniZinc::Error(oss.str());
    }
  }

  _env = new Env();
  std::stringstream errstream;
  auto* model = parse(*_env, filenames, std::vector<std::string>(), textModel, textModelName, includePaths, {},
                 false, false, false, false, errstream);
  if (model != nullptr) {
    std::vector<TypeError> typeErrors;
    _env->model(model);
    _envGuard.reset(_env);
    typecheck(*_env, model, typeErrors, false, false);
    register_builtins(*_env);
    _env->model()->checkFnValid(_env->envi(), typeErrors);
  } else {
    throw Error(errstream.str());
  }

  GCLock lock;
  ArrayLit* arg1 = new ArrayLit(Location().introduce(), std::vector<Expression*>());
  arg1->type(Type::parint(1));
  ArrayLit* arg2 = new ArrayLit(Location().introduce(), std::vector<Expression*>());
  arg2->type(Type::parfloat(1));
  Call* c = Call::a(Location().introduce(), "main", {arg1, arg2});
  _decl = _env->model()->matchFn(_env->envi(), c, false);
  if (_decl == nullptr) {
    throw Error("no matching main function found");
  }
  auto t = _decl->rtype(_env->envi(), {Type::parint(1), Type::parfloat(1)}, c, false);
  auto tt = Type::tuple(_env->envi().registerTupleType({Type::parint(1), Type::parfloat(1)}));
  if (t != tt) {
    throw Error("return type of main function does not match");
  }
}

void MznEvaluator::loadModel(const std::string& filename) {
  std::vector<std::string> filenames {filename};
  load(filenames, "", "");
}


void MznEvaluator::loadModelString(const std::string& textModel,
                                   const std::string& textModelName) {
  std::vector<std::string> filenames;
  load(filenames, textModel, textModelName);
}
    
MznEvaluator::params_t MznEvaluator::eval(const MznEvaluator::params_t& params) {
  GCLock lock;
  std::vector<Expression*> intArgs(params.first.size());
  for (unsigned int i = 0; i < params.first.size(); i++) {
    intArgs[i] = IntLit::a(params.first[i]);
  }
  ArrayLit* arg1 = new ArrayLit(Location().introduce(), intArgs);
  arg1->type(Type::parint(1));
  std::vector<Expression*> floatArgs(params.second.size());
  for (unsigned int i = 0; i < params.second.size(); i++) {
    floatArgs[i] = FloatLit::a(params.second[i]);
  }
  ArrayLit* arg2 = new ArrayLit(Location().introduce(), floatArgs);
  arg2->type(Type::parfloat(1));
  
  _decl->param(0)->flat(_decl->param(0));
  _decl->param(0)->e(arg1);
  _decl->param(1)->flat(_decl->param(1));
  _decl->param(1)->e(arg2);
  
  auto* result = eval_par(_env->envi(), _decl->e());
  
  auto* result_tuple = Expression::cast<ArrayLit>(result);
  auto* result_int = Expression::cast<ArrayLit>((*result_tuple)[0]);
  auto* result_float = Expression::cast<ArrayLit>((*result_tuple)[1]);
  
  params_t ret;
  ret.first.resize(result_int->size());
  ret.second.resize(result_float->size());
  for (unsigned int i = 0; i < result_int->size(); i++) {
    ret.first[i] = IntLit::v(Expression::cast<IntLit>((*result_int)[i])).toInt();
  }
  for (unsigned int i = 0; i < result_float->size(); i++) {
    ret.second[i] = FloatLit::v(Expression::cast<FloatLit>((*result_float)[i])).toDouble();
  }
  return ret;
}

}

