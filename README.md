# Live Link Face CSV Importer for Maya

### Instructions

Simply move the CSVImporter.mll to your plug-ins folder. The path may vary depending on the environment variables setup on your machine. The default however is "C:/Users/Name/Documents/maya/Version/plug-ins", the most common alternative path would be "C:\Program Files\Autodesk\Maya2020\bin\plug-ins". If neither work check your environment variables for MAYA_PLUG_IN_PATH and adjust your path accordingly.

Activate the plug-in in your plug-in manager, select the target mesh and import a .csv through your preferred method. The mesh needs to have the blendshapes setup already, else it will throw an error. The blendshapes also need to have the same name as in the .csv. If you want to change the names of the meshes you can create the blendshapes with the proper names and rename them afterwards. Joints follow the same rules except they can't have their name changed at all. I am considering to add the ability to change joint names in the future. If singular blendshapes/joints are missing the file will still be imported, but your mocap will most likely look odd. For now the Z Axis controls Roll, Y Yaw and X Pitch, I will add the ability to change the axis before import in the future.

Latest compiled release available for Maya 2018-2022 [here](https://github.com/ArhasGH/Live-Link-Face-CSV-to-Maya/releases/latest)

