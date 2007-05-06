
#pragma once

#include "coffldr.h"

#ifndef NULL
#define NULL 0
#endif

class DllLoader;


typedef struct Export
{
  const char*   name;
  unsigned long ordinal;
  void*         function;
  void*         track_function;
} Export;

typedef struct ExportEntry
{
  Export exp;
  ExportEntry* next;
} ExportEntry;

typedef struct _LoadedList
{
  DllLoader* pDll;
  _LoadedList* pNext;
} LoadedList;
  
class DllLoader : public CoffLoader
{
public:
  DllLoader(const char *dll, bool track = false, bool bSystemDll = false, bool bLoadSymbols = false, Export* exports = NULL);
  ~DllLoader();

  bool Load();
  void Unload();
  
  int Parse();
  int ResolveImports();
  int ResolveExport(const char*, void**);
  int ResolveExport(unsigned long ordinal, void**);

  char* GetName(); // eg "mplayer.dll"
  char* GetFileName(); // "Q:\system\mplayer\players\mplayer.dll"
  char* GetPath(); // "Q:\system\mplayer\players\"
  int IncrRef();
  int DecrRef();
  
  Export* GetExportByOrdinal(unsigned long ordinal);
  Export* GetExportByFunctionName(const char* sFunctionName);
  bool IsSystemDll() { return m_bSystemDll; }
  bool HasSymbols() { return m_bLoadSymbols && !m_bUnloadSymbols; }
  
  void AddExport(unsigned long ordinal, unsigned long function, void* track_function = NULL);
  void AddExport(char* sFunctionName, unsigned long ordinal, unsigned long function, void* track_function = NULL);
  void AddExport(char* sFunctionName, unsigned long function, void* track_function = NULL);
  void SetExports(Export* exports) { m_pStaticExports = exports; }

private:
  // Just pointers; dont' delete...
  ImportDirTable_t *ImportDirTable;
  ExportDirTable_t *ExportDirTable;
  char* m_sFileName;
  char* m_sPath;
  int m_iRefCount;
  bool m_bTrack;
  bool m_bSystemDll; // true if this dll should not be removed
  bool m_bLoadSymbols; // when true this dll should not be removed
  bool m_bUnloadSymbols;
  ExportEntry* m_pExportHead;
  Export* m_pStaticExports;
  LoadedList* m_pDlls;

  void PrintImportLookupTable(unsigned long ImportLookupTable_RVA);
  void PrintImportTable(ImportDirTable_t *ImportDirTable);
  void PrintExportTable(ExportDirTable_t *ExportDirTable);
  
  int ResolveOrdinal(char*, unsigned long, void**);
  int ResolveName(char*, char*, void **);
  char* ResolveReferencedDll(char* dll);
  int LoadExports();
  void LoadSymbols();
  void UnloadSymbols();
};
