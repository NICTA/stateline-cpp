//! A demo using Stateline to sample from a bimodal Gaussian distribution.
//!
//! \file bimodal.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \licence Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <iostream>
#include <cmath>
#include <string>
#include <thread>

#include "stateline/worker.hpp"

double gaussianDensity(const std::vector<double>& x, double mean)
{
  double normSquared = 0.0;
  for (auto i : x)
  {
    normSquared += (i - mean) * (i - mean);
  }
  return exp(-normSquared / 2.0);
}

double bimodalNLL(stateline::JobType, const std::vector<double>& x)
{
  return -log(gaussianDensity(x, -3) + gaussianDensity(x, 3));
}

int main(int argc, const char *argv[])
{
  if (argc != 2)
  {
    std::cout << "Usage: " << argv[0] << " <address of stateline server>" << std::endl;
    return 0;
  }

  stateline::runWorker(argv[1], bimodalNLL);
}
