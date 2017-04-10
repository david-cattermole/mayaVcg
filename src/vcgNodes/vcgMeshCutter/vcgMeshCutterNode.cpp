/*
 *
 */

// Local
#include <vcgNodes/vcgMeshCutter/vcgMeshCutterNode.h>

// Utils
#include <utilities/debugUtils.h>

// Function Sets

#include <maya/MFnMeshData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnMatrixData.h>


// General Includes
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MIOStream.h>

// Macros
#define MCheckStatus(status, message)  \
  if( MStatus::kSuccess != status ) {  \
    cerr << message << "\n";           \
    return status;                     \
  }


// Unique Node TypeId
MTypeId vcgMeshCutterNode::id(0x00085002); // Use a unique ID.

// Node attributes
MObject vcgMeshCutterNode::inMesh;
MObject vcgMeshCutterNode::outMesh;
MObject vcgMeshCutterNode::aEnable;
MObject vcgMeshCutterNode::aInvert;
MObject vcgMeshCutterNode::aWorldMatrix;
MObject vcgMeshCutterNode::aCutterBBoxMin;
MObject vcgMeshCutterNode::aCutterBBoxMax;
MObject vcgMeshCutterNode::aCutterMatrix;
MObject vcgMeshCutterNode::aCutterShapeType;

vcgMeshCutterNode::vcgMeshCutterNode()
{}

vcgMeshCutterNode::~vcgMeshCutterNode()
{}

MStatus vcgMeshCutterNode::compute(const MPlug &plug, MDataBlock &data)
//
//  Description:
//    This method computes the value of the given output plug based
//    on the values of the input attributes.
//
//  Arguments:
//    plug - the plug to compute
//    data - object that provides access to the attributes for this node
//
{
  MStatus status = MS::kSuccess;

  MDataHandle stateData = data.outputValue(state, &status);
  MCheckStatus(status, "ERROR getting state");

  // Check for the HasNoEffect/PassThrough flag on the node.
  //
  // (stateData is an enumeration standard in all depend nodes)
  //
  // (0 = Normal)
  // (1 = HasNoEffect/PassThrough)
  // (2 = Blocking)
  // ...
  //
  if (stateData.asShort() == 1)
  {
    MDataHandle inputData = data.inputValue(inMesh, &status);
    MCheckStatus(status, "ERROR getting inMesh");

    MDataHandle outputData = data.outputValue(outMesh, &status);
    MCheckStatus(status, "ERROR getting outMesh");

    // Simply redirect the inMesh to the outMesh for the PassThrough effect
    outputData.set(inputData.asMesh());
  }
  else
  {
    // Check which output attribute we have been asked to
    // compute. If this node doesn't know how to compute it,
    // we must return MS::kUnknownParameter
    if (plug == vcgMeshCutterNode::outMesh)
    {
      MDataHandle inputData = data.inputValue(inMesh, &status);
      MCheckStatus(status, "ERROR getting inMesh");

      MDataHandle outputData = data.outputValue(outMesh, &status);
      MCheckStatus(status, "ERROR getting outMesh");

      // Copy the inMesh to the outMesh, so you can
      // perform operations directly on outMesh
      outputData.set(inputData.asMesh());

      // Return if the node is not enabled.
      MDataHandle enableData = data.inputValue(aEnable, &status);
      MCheckStatus(status, "ERROR getting aEnable");
      if (!enableData.asBool())
      {
        return MS::kSuccess;
      }

      // Set invert
      MDataHandle invertData = data.inputValue(aInvert, &status);
      MCheckStatus(status, "ERROR getting aInvert");
      fFactory.setInvert(invertData.asBool());

      // Set world matrix
      MDataHandle worldMatrixHandle = data.inputValue(aWorldMatrix, &status);
      MCheckStatus(status, "ERROR getting aWorldMatrix");
      MMatrix worldMatrix = worldMatrixHandle.asMatrix();
      fFactory.setWorldMatrix(worldMatrix);

      // Set cutter bbox min
      MDataHandle cutterBBoxMinHandle = data.inputValue(aCutterBBoxMin, &status);
      MCheckStatus(status, "ERROR getting aCutterMatrix");
      MFloatPoint cutterBBoxMin = cutterBBoxMinHandle.asFloatVector();
      fFactory.setCutterBBoxMin(cutterBBoxMin);

      // Set cutter bbox max
      MDataHandle cutterBBoxMaxHandle = data.inputValue(aCutterBBoxMax, &status);
      MCheckStatus(status, "ERROR getting aCutterMatrix");
      MFloatPoint cutterBBoxMax = cutterBBoxMaxHandle.asFloatVector();
      fFactory.setCutterBBoxMax(cutterBBoxMax);

      // Set cutter matrix
      MDataHandle cutterMatrixHandle = data.inputValue(aCutterMatrix, &status);
      MCheckStatus(status, "ERROR getting aCutterMatrix");
      MMatrix cutterMatrix = cutterMatrixHandle.asMatrix();
      fFactory.setCutterMatrix(cutterMatrix);

      // Set cutter shape type
      MDataHandle cutterShapeTypeData = data.inputValue(aCutterShapeType, &status);
      MCheckStatus(status, "ERROR getting aCutterShapeType");
      fFactory.setCutterShapeType(static_cast<Shapes>(cutterShapeTypeData.asShort()));

      // Get Mesh object
      MObject mesh = outputData.asMesh();

      // Set the mesh object and component List on the factory
      fFactory.setMesh(mesh);

      // Now, perform the meshCutter
      status = fFactory.doIt();

      // Mark the output mesh as clean
      outputData.setClean();
    }
    else
    {
      status = MS::kUnknownParameter;
    }
  }

  return status;
}

void *vcgMeshCutterNode::creator()
//
//  Description:
//    this method exists to give Maya a way to create new objects
//      of this type. 
//
//  Return Value:
//    a new object of this type
//
{
  return new vcgMeshCutterNode();
}

MStatus vcgMeshCutterNode::initialize()
//
//  Description:
//    This method is called to create and initialize all of the attributes
//      and attribute dependencies for this node type.  This is only called 
//    once when the node type is registered with Maya.
//
//  Return Values:
//    MS::kSuccess
//    MS::kFailure
//    
{
  MStatus status;

  MFnTypedAttribute attrFn;
  MFnMatrixAttribute matFn;
  MFnEnumAttribute enumFn;
  MFnNumericAttribute numFn;

  aEnable = numFn.create("enable", "enable",
                         MFnNumericData::kBoolean, true);
  status = numFn.setDefault(true);
  status = numFn.setStorable(true);
  status = numFn.setKeyable(true);
  status = numFn.setChannelBox(true);
  status = numFn.setHidden(false);
  status = addAttribute(aEnable);
  CHECK_MSTATUS(status);

  aInvert = numFn.create("invert", "invert",
                         MFnNumericData::kBoolean, false);
  status = numFn.setDefault(false);
  status = numFn.setStorable(true);
  status = numFn.setKeyable(false);
  status = numFn.setChannelBox(true);
  status = numFn.setHidden(false);
  status = addAttribute(aInvert);
  CHECK_MSTATUS(status);

  aWorldMatrix = matFn.create("worldMatrix", "worldMatrix");
  status = matFn.setStorable(true);
  status = matFn.setKeyable(false);
  status = matFn.setHidden(false);
  status = addAttribute(aWorldMatrix);
  CHECK_MSTATUS(status);

  aCutterBBoxMin = numFn.create("cutterBBoxMin", "cutterBBoxMin",
                                MFnNumericData::k3Float, -0.5);
  status = numFn.setStorable(true);
  status = numFn.setKeyable(false);
  status = numFn.setChannelBox(true);
  status = numFn.setHidden(false);
  status = addAttribute(aCutterBBoxMin);
  CHECK_MSTATUS(status);

  aCutterBBoxMax = numFn.create("cutterBBoxMax", "cutterBBoxMax",
                                MFnNumericData::k3Float, 0.5);
  status = numFn.setStorable(true);
  status = numFn.setKeyable(false);
  status = numFn.setChannelBox(true);
  status = numFn.setHidden(false);
  status = addAttribute(aCutterBBoxMax);
  CHECK_MSTATUS(status);

  aCutterMatrix = matFn.create("cutterMatrix", "cutterMatrix");
  status = matFn.setStorable(true);
  status = matFn.setKeyable(false);
  status = matFn.setHidden(false);
  status = addAttribute(aCutterMatrix);
  CHECK_MSTATUS(status);

  aCutterShapeType = enumFn.create("cutterShapeType", "cutterShapeType", Shapes::kCube);
  status = enumFn.addField("None", Shapes::kNone);
  status = enumFn.addField("Cube", Shapes::kCube);
  status = enumFn.addField("Sphere", Shapes::kSphere);
  status = enumFn.setStorable(true);
  status = enumFn.setKeyable(false);
  status = enumFn.setHidden(false);
  status = addAttribute(aCutterShapeType);
  CHECK_MSTATUS(status);

  inMesh = attrFn.create("inMesh", "im",
                         MFnMeshData::kMesh);
  attrFn.setStorable(true);  // To be stored during file-save
  status = addAttribute(inMesh);
  CHECK_MSTATUS(status);

  // Attribute is read-only because it is an output attribute
  outMesh = attrFn.create("outMesh", "om",
                          MFnMeshData::kMesh);
  attrFn.setStorable(false);
  attrFn.setWritable(false);
  status = addAttribute(outMesh);
  CHECK_MSTATUS(status);

  // Attribute affects
  status = attributeAffects(inMesh, outMesh);
  CHECK_MSTATUS(status);
  status = attributeAffects(aEnable, outMesh);
  CHECK_MSTATUS(status);
  status = attributeAffects(aInvert, outMesh);
  CHECK_MSTATUS(status);
  status = attributeAffects(aWorldMatrix, outMesh);
  CHECK_MSTATUS(status);
  status = attributeAffects(aCutterBBoxMin, outMesh);
  CHECK_MSTATUS(status);
  status = attributeAffects(aCutterBBoxMax, outMesh);
  CHECK_MSTATUS(status);
  status = attributeAffects(aCutterMatrix, outMesh);
  CHECK_MSTATUS(status);
  status = attributeAffects(aCutterShapeType, outMesh);
  CHECK_MSTATUS(status);

  return MS::kSuccess;

}
