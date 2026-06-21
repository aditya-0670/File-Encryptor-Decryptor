#include "src/app/cli/CliApplication.hpp"

int main(int argc, char* argv[]) {
    vault::cli::CliApplication app;
    return app.run(argc, argv);
}
