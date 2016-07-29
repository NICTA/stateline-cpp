//! A demo using Stateline to sample from a standard normal distribution.
//!
//! This file aims to be a tutorial on setting up a MCMC simulation using
//! the C++ worker API.
//!
//! \file simple.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \licence Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <string>
#include <thread>
#include <iostream>

#include "stateline/worker.hpp"

double gaussianNLL(stateline::JobType, const std::vector<double>& x)
{
  double squaredNorm = 0.0;
  for (auto i : x)
  {
    squaredNorm += i * i;
  }
  return squaredNorm;
}

int main(int argc, const char *argv[])
{
  if (argc != 2)
  {
    std::cout << "Usage: " << argv[0] << " <address of stateline server>" << std::endl;
    return 0;
  }

  stateline::runWorker(argv[1], gaussianNLL);
}
