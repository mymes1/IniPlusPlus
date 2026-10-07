// Loads a built Android extension .so with dlopen() and resolves every entry point the Fusion 2.5
// Android runtime looks up, under both spellings of the extension name.  Used by
// tools/smoke_build_so.sh: nm proves the symbols exist in the file, dlsym proves the dynamic
// linker (and therefore the runtime's native loader) can actually find them.
//
// Usage: load-probe <path to .so>

#include <cstdio>
#include <dlfcn.h>
#include <string>
#include <vector>

int main(int const argc, char const* const argv[])
{
	if(argc != 2)
	{
		std::fprintf(stderr, "usage: %s <extension.so>\n", argv[0]);
		return 2;
	}
	void* const handle{dlopen(argv[1], RTLD_NOW | RTLD_LOCAL)};
	if(!handle)
	{
		std::fprintf(stderr, "FAIL  dlopen(%s): %s\n", argv[1], dlerror());
		return 1;
	}

	char const* const entry_points[]{
		"extInit", "createRunObject", "getNumberOfConditions", "destroyRunObject",
		"handleRunObject", "action", "condition", "expression",
	};

	int failures{};
	for(auto const* const name : std::vector<char const*>{"CRunINI++", "CRunIniPlusPlus"})
	{
		for(auto const* const entry : entry_points)
		{
			auto const symbol{std::string{name} + "_" + entry};
			if(dlsym(handle, symbol.c_str()))
			{
				std::printf("  ok   dlsym %s\n", symbol.c_str());
			}
			else
			{
				std::printf("FAIL  dlsym %s: %s\n", symbol.c_str(), dlerror());
				++failures;
			}
		}
	}

	// The two exports the runtime needs to know about the object itself
	using conditions_fn = int (*)();
	auto const conditions{reinterpret_cast<conditions_fn>(dlsym(handle, "CRunINI++_getNumberOfConditions"))};
	if(conditions && conditions() == 20)
	{
		std::printf("  ok   CRunINI++_getNumberOfConditions() == 20\n");
	}
	else
	{
		std::printf("FAIL  CRunINI++_getNumberOfConditions() returned %d\n", conditions ? conditions() : -1);
		++failures;
	}
	return failures == 0 ? 0 : 1;
}
