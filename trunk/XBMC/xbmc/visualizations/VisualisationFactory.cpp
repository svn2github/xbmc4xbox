#include "VisualisationFactory.h"
#include "../cores/DllLoader/dll.h"


CVisualisationFactory::CVisualisationFactory()
{
}
CVisualisationFactory::~CVisualisationFactory()
{
}
extern "C" void __declspec(dllexport) get_module(struct Visualisation* pVisz);

CVisualisation* CVisualisationFactory::LoadVisualisation(const CStdString& strVisz) const
{
	DllLoader* pDLL = new DllLoader(strVisz.c_str(), true);
	if( !pDLL->Parse() )
	{
		delete pDLL;
		return NULL;
	}
	if( !pDLL->ResolveImports()  )
	{
	}

	void (__cdecl* pGetModule)(struct Visualisation*);
	struct Visualisation* pVisz = (struct Visualisation*)malloc(sizeof(struct Visualisation));
	void* pProc;
	pDLL->ResolveExport("get_module", &pProc);
	if (!pProc)
	{
		delete pDLL;
		return NULL;
	}
	pGetModule=(void (__cdecl*)(struct Visualisation*))pProc;
	pGetModule(pVisz);
	return new CVisualisation(pVisz,pDLL);
}
