#pragma once

#ifdef _MSC_VER 
//not #if defined(_WIN32) || defined(_WIN64) because we have strncasecmp in mingw
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MFnPlugin.h>
#include <maya/MString.h>
#include <maya/MVector.h>
#include <maya/MStringArray.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionList.h>
#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MNamespace.h>
#include <maya/MDagPath.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MDGModifier.h>
#include <maya/MItDependencyGraph.h>

#include <fstream>
#include <iostream>
#include <ios>

#include "rapidcsv.h"

class csvImport :public MPxFileTranslator 
{
private:
	std::vector<std::string> timecodes;
	std::vector<std::string> target;
	int offset = 0;
	int s_offset = 0;
	int get_frame(std::vector<std::string>::iterator time);
	std::vector<std::string> find_frame(std::vector<std::string>::iterator iter);
	std::string convertEuler(std::string input, const char** index);
public:
	csvImport();
	~csvImport();

	bool haveReadMethod() const override { return true; }
	bool canBeOpened() const override { return false; }
	static void* creator() { return new csvImport(); }

	MFileKind identifyFile(const MFileObject& fileName, const char* buffer, short size) const override;
	MStatus reader(const MFileObject& file, const MString& optionsString, MPxFileTranslator::FileAccessMode mode) override;
};

MStatus initializePlugin(MObject obj);
MStatus uninitializePlugin(MObject obj);
