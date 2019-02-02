#include "Application.hpp"

int main(int argc, char** argv)
{
	Application ui;
	if(argc < 2) ui.Initialize();
	else ui.InitializeFromFile(argv[1]);
	ui.Run();

	return EXIT_SUCCESS;
}
