#include "BVH/SBVHBuilder.hpp"
#include "Tracer/OglInterface.hpp"
#include "Util/Platform.hpp"
#include "BVH/WideBVH.hpp"
#include "BVH/WideBVHBuilder.hpp"
#include <cstring>
#include <string>

int main(int argc, char** argv)
{
	argc --; argv ++;
	if(argc == 0) return EXIT_FAILURE;
	Platform pf{*argv};
	if(pf.Invalid())
	{
		//print help
		return EXIT_FAILURE;
	}

	OglInterface ui;

	{
		Scene scene{pf.GetObjFilename()};
		WideBVH wbvh{pf.GetBVHFilename()};

		//create wide-bvh
		if(wbvh.Empty() || wbvh.GetConfig() != pf.GetBVHConfig())
		{
			SBVH sbvh;
			SBVHBuilder{pf.GetBVHConfig(), &sbvh, scene}.Run();
			WideBVHBuilder{pf.GetBVHConfig(), &wbvh, sbvh}.Run();
			wbvh.Save(pf.GetBVHFilename());
		}

		ui.Initialize(&pf, scene, wbvh);
	}
	ui.Run();

	return EXIT_SUCCESS;
}
