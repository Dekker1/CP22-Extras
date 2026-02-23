/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */

/*
 *  Main authors:
 *     Guido Tack <guido.tack@monash.edu>
 */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <memory>
#include <minizinc/file_utils.hh>
#include <minizinc/model.hh>

namespace MiniZinc {

  /// Evaluate a callback par function. The function must have the following signature:
  /// function tuple(array[int] of int, array[int] of float): main(array[int] of int: x, array[int] of float: y)
  class MznEvaluator {
  public:
    /// The parameter and return type of the function
    using params_t = std::pair<std::vector<long long>,std::vector<double>>;
  
    /// Constructor
    explicit MznEvaluator(std::string stdlibDir = FileUtils::share_directory());

    /// Load model with main function from a file
    void loadModel(const std::string& filename);

    /// Load model with main function from a string
    void loadModelString(const std::string& textModel,
                         const std::string& textModelName = std::string("model.mzn"));
    
    /// Evaluate main function and return result
    params_t eval(const params_t& params);
    
  private:
    /// Helper function for loading the model
    void load(const std::vector<std::string> filenames,
              const std::string& textModel, const std::string& textModelName);

    /// Location of the MiniZinc standard library
    std::string _stdlibDir;
    /// The environment
    Env* _env;
    /// Destroy environment when finished
    std::unique_ptr<Env> _envGuard;
    /// The main function to be called
    FunctionI* _decl;
  };


}  // namespace MiniZinc
