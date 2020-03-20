#include <thread>
#include "functionapi.h"

int clientThread(std::string configFile) {
  Configuration configuration = Configuration(configFile);

  uint16_t port = stoi(configuration.get("port"), nullptr, 10);
  std::string serverAddress = configuration.get("serveraddress");

  std::cout << configuration.toString();

  socketClient client = initializeFunctionClient(port, serverAddress);

  client.sendSessionStart();

  std::size_t params[5] = {100, 250, 500, 750, 1000};
  std::size_t size;

  for (std::size_t& n : params) {
    client.sendTag("Starting for n := " + std::to_string(n));
    size = n * n;

    double* a = new double[size];
    double* b = new double[size];
    double* c = new double[size];

    srand(time(NULL));
    for (std::size_t i = 0; i < size; i++) {
      a[i] = rand() % 20 + 1;
      b[i] = rand() % 20 + 1;
    }

    client.sendTag("Starting matrix mult");

    for (std::size_t row = 0; row < n; row++) {
      std::size_t curr_line = row * n;

      for (std::size_t k = 0; k < n; k++) {
        std::size_t row_idx = curr_line + k;
        std::size_t curr_col = n * k;

        for (std::size_t col = 0; col < n; col++) {
          std::size_t col_idx = curr_col + col;
          c[curr_line + col] += a[row_idx] * b[col_idx];
        }
      }
    }

    client.sendTag("End matrix mult");

    delete a;
    delete b;
    delete c;
  }

  client.sendSessionEnd();
  return 0;
}

int main(int argc, char** argv) {

  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <config file> <output file>"
              << std::endl;
    exit(EXIT_FAILURE);
  }
  std::string configFile(argv[1]);

  std::thread runHandler(clientThread, configFile);
  runHandler.join();
}
