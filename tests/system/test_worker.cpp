#include <vector>
#include <iostream>

#include <stateline/worker.hpp>

double nll(stateline::JobType, const std::vector<double>&)
{
  return 0.0;
}

int main(int argc, const char *argv[])
{
  if (argc != 2)
  {
    std::cout << "Usage: " << argv[0] << " <address of stateline server>" << std::endl;
    return 0;
  }

  stateline::runWorker(argv[1], nll);
}
