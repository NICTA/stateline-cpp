# stateline-cpp

Library for building C++ Stateline workers.

## Usage

For every Stateline worker, launch a corresponding `stateline-agent` with a different socket.

## Example

The following code gives a minimal example of building a stateline
worker in C++ with a custom likelihood function `gaussianNLL`:

```c++
// Compile command:
// g++ -isystem ${STATELINE_INSTALL_DIR}/include -L${STATELINE_INSTALL_DIR}/lib
//   -std=c++11 -o myworker myworker.cpp -lstatelineclient -lzmq -lpthread

#include <thread>
#include <chrono>

#include <stateline/worker.hpp>

double gaussianNLL(stateline::JobType type, const std::vector<double>& x)
{
  double squaredNorm = 0.0;
  for (auto i : x)
  {
    squaredNorm += i*i;
  }
  return 0.5*squaredNorm;
}

int main(int argc, char* argv[])
{
  if (argc != 2)
    return 1;
    
  stateline::runWorker(argv[1], gaussianNLL);
}
```
