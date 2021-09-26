#include "csvImport.h"

// both the constructor and destructor don't need to do anything in this case
csvImport::csvImport(){}
csvImport::~csvImport(){}

// the timecode is equal to the time the recording took place, we need to use this method to convert it to 0
int csvImport::get_frame(std::vector<std::string>::iterator time)
{
	// split the timecode
	std::vector<std::string> out = find_frame(time);
	// get the frames
	int output = std::stoi(out.at(3));
	// get the seconds
	int seconds = std::stoi(out.at(2));
	// if this is the first frame we need to deduct it by itself -1 so we end up with frame 1
	if (time == timecodes.begin()) {
		offset = (output - 1);
		// we also need to keep track of the seconds as the frames go back to 0 once they reach 59
		s_offset = std::stoi(find_frame(timecodes.begin()).at(2));
	}
	// simple math to get the correct frame count
	output += 60*(seconds - s_offset);
	return output - offset;
}

// method for splitting timecode
std::vector<std::string> csvImport::find_frame(std::vector<std::string>::iterator iter) {
	std::stringstream ss(*iter);
	std::vector<std::string> out;
	std::string token;
	char delim = ':';

	while (std::getline(ss, token, delim)) {
		out.push_back(token);
	}
	return out;
}

// incredibly scuffed way of getting the correct rotation from the name, there's probably a better way than to use ** but it works
std::string csvImport::convertEuler(std::string input, const char** index) {
	std::string temp;
	if (std::string::npos != input.find("Yaw")) {
		*index = "ry";
		for (int e = 0; e < input.find("Yaw"); e++) {
			temp += input[e];
		}
	}
	else if (std::string::npos != input.find("Pitch")) {
		*index = "rx";
		for (int e = 0; e < input.find("Pitch"); e++) {
			temp += input[e];
		}
	}
	else if (std::string::npos != input.find("Roll")) {
		*index = "rz";
		for (int e = 0; e < input.find("Roll"); e++) {
			temp += input[e];
		}
	}
	return temp;
}

MStatus csvImport::reader(const MFileObject& file, const MString& optionsString, MPxFileTranslator::FileAccessMode mode)
{
	// open the received .csv with rapidcsv
	rapidcsv::Document doc(file.expandedFullName().asChar(), rapidcsv::LabelParams(0, 0));
	// get names of target objects
	target = doc.GetColumnNames();
	// same for timecodes
	timecodes = doc.GetRowNames();

	MObject dag;
	MFnAnimCurve afn;
	MStringArray alias;
	int index = -1;
	
	// grab blendshape from selection
	MSelectionList sel;
	MDagPath shape;
	MObject component;
	MGlobal::getActiveSelectionList(sel);
	// simple exception handling
	if (!(sel.length() > 0)) {
		MGlobal::displayError("Selection is empty");
		return MS::kFailure;
	}
	sel.getDagPath(0, shape);
	// we need to clear selectionlist as we use it later on
	sel.clear();
	if (!shape.hasFn(MFn::kMesh)) {
		MGlobal::displayError("Selected object not of type mesh");
		return MS::kFailure;
	}
	bool bs_found = false;
	shape.extendToShapeDirectlyBelow(0);
	dag = shape.node();
	MFnDependencyNode DFn;
	// create an iterator to find the blendshape
	MItDependencyGraph DGit = MItDependencyGraph(dag, MFn::kBlendShape, MItDependencyGraph::kUpstream, MItDependencyGraph::kDepthFirst, MItDependencyGraph::kPlugLevel);
	while (!DGit.isDone()) {
		// get iterators current item
		MObject temp = DGit.currentItem();
		// check if that object is a blendshape
		if (temp.hasFn(MFn::kBlendShape)) {
			bs_found = true;
			dag = temp;
			break;
		}
		DGit.next();
	}
	// make sure it actually found one
	if (!bs_found) {
		MGlobal::displayError("No blendshape found");
		return MS::kFailure;
	}
	MObject curve;

	// attach a MFnDependencyNode to the blendshape
	MFnDependencyNode dfn(dag);
	// find the weight attribute
	MPlug plug = dfn.findPlug("weight", false);
	// unfortunately attributes have to be accessed via indices so we need to find the correct index via the alias. We just store those in a String Array
	dfn.getAliasList(alias);
	MObject dummy;
	// iterate through the blendshapes / left to right
	for (auto t = target.begin() + 1; t != target.end(); t++) {
		if (dfn.findAlias(t->data(), dummy)) {
			// iterate through the alias, 2 steps at a time as the next value corresponds to the current target
			for (unsigned int i = 0; i < alias.length(); i+=2) {
				// check if alias is the wanted one
				if (alias[i] == t->data()) {
					// if so grab the index, unfortunately the output would be something like weight[12] so we need to just extract the index
					index = atoi(alias[i + 1].asChar()+7);
					// finish looping
					break;
				}
			}
			// check if attribute is already connected
			if (plug[index].isConnected()) {
				// make sure the source is an animcurve
				if (curve.apiType() != MFn::kAnimCurve) {
					// if not delete the connection
					MPlug source = plug[index].source();
					MDGModifier dgm;
					dgm.disconnect(source, plug[index]);
					curve = afn.create(plug[index], afn.kAnimCurveTU, NULL);
				}
				// if it is use it
				curve = plug[index].source().node();
				afn.setObject(curve);
			}
			// if not create the animcurve
			else {
				curve = afn.create(plug[index], afn.kAnimCurveTU, NULL);
			}
			// iterate through the timecodes / top to bottom
			for (auto time = timecodes.begin(); time != timecodes.end(); time++) {
				// convert that to frames
				int frame = get_frame(time);
				// grab the value
				float value = doc.GetCell<float>(*t, *time);
				// add the keyframe, recording is in 60 fps
				afn.addKey(MTime(frame, MTime::k60FPS), value);
			}
		}
		// do joint related things
		else {
			const char* rotation = "";
			MStatus status = sel.add(convertEuler(t->data(), &rotation).c_str());
			if (status != MStatus::kSuccess) {
				MGlobal::displayWarning(t->data() + MString(" not found, skipping"));
				continue;
			}
			MObject joint;
			sel.getDependNode(0, joint);
			DFn.setObject(joint);
			MPlug r = DFn.findPlug(rotation, false);
			afn.create(r, afn.kAnimCurveTA, NULL);
			for (auto time = timecodes.begin(); time != timecodes.end(); time++) {
				// convert that to frames
				int frame = get_frame(time);
				// grab the value
				float value = doc.GetCell<float>(*t, *time);
				// add the keyframe, recording is in 60 fps
				afn.addKey(MTime(frame, MTime::k60FPS), value);
			}
			sel.clear();
		}
	}
	return MS::kSuccess;
}

// method that tells maya that this importer knows how to handle the file type
csvImport::MFileKind csvImport::identifyFile(const MFileObject & fileName, const char * buffer, short size) const
{
	const char *name = fileName.resolvedName().asChar();
	int nameLength = (int)strlen(name);
	// check if it ends with .csv, if so tell maya that you know the file type
	if ((nameLength > 4) && !strcasecmp(name + nameLength - 4, ".csv")) {
		return kIsMyFileType;
	}
	return kNotMyFileType;
}

// usual initializePlugin function necessary for any plugin
MStatus initializePlugin(MObject obj)
{
	MStatus   status;
	MFnPlugin plugin(obj, "Arhas", "3.0", "Any");

	status = plugin.registerFileTranslator("Csv",
		"",
		csvImport::creator);
	if (!status)
	{
		status.perror("registerFileTranslator");
		return status;
	}
	return status;
}

// usual uninitializePlugin function necessary for any plugin
MStatus uninitializePlugin(MObject obj)
{
	MStatus   status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterFileTranslator("Csv");
	if (!status)
	{
		status.perror("deregisterFileTranslator");
		return status;
	}

	return status;
}
