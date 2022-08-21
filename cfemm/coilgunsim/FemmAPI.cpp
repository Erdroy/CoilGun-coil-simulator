#include <FemmAPI.h>

#include <femmconstants.h>
#include <fsolver.h>
#include <fmesher.h>
#include <fpproc.h>
#include <MatlibReader.h>

#include <memory>
#include <sstream>

void FemmAPI::femm_init(const char* file)
{
    doc = std::make_shared<femm::FemmProblem>(femm::FileType::MagneticsFile);
    doc->pathName = file;
    
    mesher = std::make_shared<fmesher::FMesher>(doc);
    postProcessor = std::make_shared<FPProc>();
}

void FemmAPI::femm_save(const char* file)
{
    (void) doc->saveFEMFile(file);
}

void FemmAPI::femm_close()
{
    doc.reset();
    mesher.reset();
    postProcessor.reset();
}

void FemmAPI::smartmesh(bool enable)
{
    doc->DoSmartMesh = enable;
}

void FemmAPI::mi_probdef(int frequency, femm::LengthUnit lengthUnits, femm::ProblemType problemType, double precision, double depth, double min_angle)
{
    doc->Frequency = frequency;
    doc->LengthUnits = lengthUnits;
    doc->problemType = problemType;
    doc->Precision = precision;
    doc->Depth = depth;
    doc->MinAngle = min_angle;
}

void FemmAPI::mi_getmaterial(const char* matname)
{
    femm::MatlibReader reader( doc->filetype );
    std::stringstream err;
    if ( reader.parse("matlib.dat", err, matname) == femm::MatlibParseResult::OK )
    {
        auto prop = reader.takeMaterial(matname);
        doc->blockproplist.push_back(std::unique_ptr<femm::CMaterialProp>(prop));
        doc->updateBlockMap();
    }
}

void FemmAPI::mi_addnode(double x, double y)
{
    double d;
    if ((int)doc->nodelist.size()<2) {
        d=1.e-08;
    } else {
        CComplex p0,p1,p2;
        p0=doc->nodelist[0]->CC();
        p1=p0;
        for(int i=1; i< (int)doc->nodelist.size(); i++)
        {
            p2=doc->nodelist[i]->CC();
            if(p2.re<p0.re) p0.re=p2.re;
            if(p2.re>p1.re) p1.re=p2.re;
            if(p2.im<p0.im) p0.im=p2.im;
            if(p2.im>p1.im) p1.im=p2.im;
        }
        d=abs(p1-p0)*CLOSE_ENOUGH;
    }
    doc->addNode(x,y,d);
}

void FemmAPI::mi_addarc(double sx, double sy, double ex, double ey, double angle, double maxseg)
{
    femm::CArcSegment asegm;
    asegm.n0 = doc->closestNode(sx,sy);
    asegm.n1 = doc->closestNode(ex,ey);
    doc->nodelist[asegm.n1]->ToggleSelect();

    asegm.MaxSideLength = maxseg;
    asegm.ArcLength = angle;

    doc->addArcSegment(asegm);
    doc->unselectAll();
}

void FemmAPI::mi_selectnode(double x, double y)
{
    int node = doc->closestNode(x,y);
    doc->nodelist[node]->ToggleSelect();
}

void FemmAPI::mi_clearselected()
{
    doc->unselectAll();
}

void FemmAPI::mi_setnodeprop(int group_id, const char* boundary_marker_name)
{
    int nodepropidx = -1;
    std::string nodeprop = "<none>";
    if (doc->nodeMap.count(nodeprop) == 0 && boundary_marker_name != nullptr)
    {
        nodeprop = boundary_marker_name;
        nodepropidx = doc->nodeMap[nodeprop];
    }
    
    for(int i=0; i<(int)doc->nodelist.size(); i++)
    {
        if(doc->nodelist[i]->IsSelected)
        {
            doc->nodelist[i]->InGroup = group_id;
            doc->nodelist[i]->BoundaryMarker = nodepropidx;
            doc->nodelist[i]->BoundaryMarkerName = nodeprop;
        }
    }
}

void FemmAPI::mi_modifymaterial(const char* matname, int prop_id, void* value)
{
    for (auto && prop: doc->blockproplist)
    {
        const auto mat = dynamic_cast<femm::CMSolverMaterialProp*>(prop.get());
        
        if (prop->BlockName == matname) {
            switch (prop_id)
            {
            case 0:
                prop->BlockName = *static_cast<const char**>(value);
                break;
            case 13:
                mat->WireD = *static_cast<double*>(value);
                break;
            default: break;
            }
            break;
        }
    }
    doc->updateBlockMap();
}

void FemmAPI::mi_addcircprop(const char* circuit_name, int amps, int circuit_type)
{
    std::unique_ptr<femm::CMCircuit> circuit = std::make_unique<femm::CMCircuit>();
    circuit->CircName = circuit_name;
    circuit->Amps = amps;
    circuit->CircType = circuit_type;
    doc->circproplist.push_back(std::move(circuit));
    doc->updateCircuitMap();
}

void FemmAPI::mi_addsegment(double sx, double sy, double ex, double ey)
{
    doc->addSegment(doc->closestNode(sx,sy), doc->closestNode(ex,ey));
}

void FemmAPI::mi_addblocklabel(double x, double y)
{
    double d;
    if (doc->nodelist.size()<2)
        d = 1.e-08;
    else{
        CComplex p0,p1,p2;
        p0 = doc->nodelist[0]->CC();
        p1 = p0;
        for (int i=1; i<(int)doc->nodelist.size(); i++)
        {
            p2 = doc->nodelist[i]->CC();
            if(p2.re<p0.re) p0.re = p2.re;
            if(p2.re>p1.re) p1.re = p2.re;
            if(p2.im<p0.im) p0.im = p2.im;
            if(p2.im>p1.im) p1.im = p2.im;
        }
        d = abs(p1-p0)*CLOSE_ENOUGH;
    }
    doc->addBlockLabel(x,y,d);
}

void FemmAPI::mi_selectlabel(double x, double y)
{
    if (doc->labellist.empty())
        return;

    int node = doc->closestBlockLabel(x,y);
    doc->labellist[node]->ToggleSelect();
}

void FemmAPI::mi_setblockprop(const char* blocktype, bool automesh, double meshsize, const char* incircuit, double magdirection, int group, int turns)
{
    int blocktypeidx = -1;
    int incircuitidx = -1;

    if (doc->blockMap.count(blocktype))
        blocktypeidx = doc->blockMap[blocktype];
    
    if (doc->circuitMap.count(incircuit))
        incircuitidx = doc->circuitMap[incircuit];
    
    if (turns==0) turns = 1;

    for (int i=0; i<(int) doc->labellist.size(); i++)
    {
        femm::CMBlockLabel *labelPtr = dynamic_cast<femm::CMBlockLabel*>(doc->labellist[i].get());
        
        if (labelPtr != nullptr && labelPtr->IsSelected)
        {
            labelPtr->MaxArea = PI*meshsize*meshsize/4.;
            labelPtr->MagDir = magdirection;
            labelPtr->BlockTypeName = blocktype;
            labelPtr->BlockType = blocktypeidx;
            labelPtr->InCircuitName = incircuit;
            labelPtr->InCircuit = incircuitidx;
            labelPtr->InGroup = group;
            labelPtr->Turns = turns;
            //labelPtr->MagDirFctn = magdirfctn;
            if(automesh)
                labelPtr->MaxArea = 0;
        }
    }
}

void FemmAPI::mi_movetranslate(double x, double y)
{
    femm::EditMode editAction;
    editAction = doc->defaultEditMode();

    doc->updateUndo();
    doc->translateMove(x,y,editAction);

    mesher->meshline.clear();
    mesher->meshnode.clear();
    mesher->greymeshline.clear();
}

void FemmAPI::mi_selectgroup(int group)
{
    for (int i=0; i<(int)doc->nodelist.size(); i++)
    {
        if(doc->nodelist[i]->InGroup==group)
            doc->nodelist[i]->IsSelected=true;
    }

    // select segments
    for(int i=0; i<(int)doc->linelist.size(); i++)
    {
        if(doc->linelist[i]->InGroup==group)
            doc->linelist[i]->IsSelected=true;
    }

    // select arc segments
    for(int i=0; i<(int)doc->arclist.size(); i++)
    {
        if(doc->arclist[i]->InGroup==group)
            doc->arclist[i]->IsSelected=true;
    }

    // select blocks
    for(int i=0; i<(int)doc->labellist.size(); i++)
    {
        if(doc->labellist[i]->InGroup==group)
            doc->labellist[i]->IsSelected=true;
    }

    // set default edit mode
    doc->setDefaultEditMode(femm::EditMode::EditGroup);
}

void FemmAPI::mi_modifycircprop(const char* circuit, int prop_id, void* value)
{
    auto searchResult = doc->circuitMap.find(circuit);
    if (searchResult == doc->circuitMap.end())
        return;

    int idx = searchResult->second;
    femm::CMCircuit *prop = dynamic_cast<femm::CMCircuit*>(doc->circproplist[idx].get());
    switch(prop_id)
    {
    case 0:
        prop->CircName = *static_cast<const char**>(value);
        break;
    case 1:
        prop->Amps = *static_cast<int*>(value);
        break;
    case 2:
        prop->CircType = *static_cast<int*>(value);
        break;
    default: break;
    }
}

void FemmAPI::mi_saveas(const char* filename)
{
    (void) doc->saveFEMFile(filename);
}

int FemmAPI::mi_analyze()
{
    if (doc->problemType==femm::AXISYMMETRIC)
    {
        // check to see if all of the input points are on r>=0 for axisymmetric problems.
        for (int k=0; k<(int)doc->nodelist.size(); k++)
        {
            if (doc->nodelist[k]->x < -(1.e-6))
                return 0;
        }

        // check to see if all block defined to be in an axisymmetric external region are linear.
        bool hasAnisotropicMaterial = false;
        bool hasExteriorProps = true;
        for (int k=0; k<(int)doc->labellist.size(); k++)
        {
            if (doc->labellist[k]->IsExternal)
            {
                if ((doc->extRo==0) || (doc->extRi==0))
                    hasExteriorProps = false;

                for(int i=0; i<(int)doc->blockproplist.size(); i++)
                {
                    if (doc->labellist[k]->BlockTypeName == doc->blockproplist[i]->BlockName)
                    {
                        femm::CMMaterialProp *prop = dynamic_cast<femm::CMMaterialProp*>(doc->blockproplist[i].get());
                        if (prop->BHpoints!=0)
                            hasAnisotropicMaterial = true;
                        else if(prop->mu_x != prop->mu_y)
                            hasAnisotropicMaterial = true;
                    }
                }
            }
        }
        if (hasAnisotropicMaterial)
            return 0;

        if (!hasExteriorProps)
            return 0;
    }

    std::string pathName = doc->pathName;
    if (pathName.empty())
        return 0;
    if (!doc->saveFEMFile(pathName))
        return 0;
    if (!doc->consistencyCheckOK())
        return 0;

    //BeginWaitCursor();
    // allow setting verbosity from lua:
    mesher->Verbose = false;
    if (mesher->HasPeriodicBC()){
        if (mesher->DoPeriodicBCTriangulation(pathName) != 0)
        {
            mesher->problem->unselectAll();
            return 0;
        }
    }
    else{
        if (mesher->DoNonPeriodicBCTriangulation(pathName) != 0)
        {
            return 0;
        }
    }

    FSolver theFSolver;
    // filename.fem -> filename
    std::size_t dotpos = doc->pathName.find_last_of(".");
    theFSolver.PathName = doc->pathName.substr(0,dotpos);
    //theFSolver.WarnMessage = &PrintWarningMsg;
    //theFSolver.PrintMessage = &PrintWarningMsg;
    // not supported yet, but set the previous solution so that we can detect this case afterwards:
    theFSolver.previousSolutionFile = doc->previousSolutionFile;
    if (!theFSolver.LoadProblemFile())
        return 0;
    
    if (!theFSolver.runSolver(false))
    {
        return 0;
    }
    
    return 1;
}

int FemmAPI::mi_loadsolution()
{
    std::size_t dotpos = doc->pathName.find_last_of(".");
    std::string solutionFile = doc->pathName.substr(0,dotpos);
    solutionFile += femm::outputExtensionForFileType(doc->filetype);

    if(postProcessor)
        postProcessor.reset();
    postProcessor = std::make_shared<FPProc>();
    if (!postProcessor->OpenDocument(solutionFile))
    {
        return 0;
    }

    return 1;
}

void FemmAPI::mo_groupselectblock(int group)
{
    if (!postProcessor->meshelem.empty())
    {
        for (auto &block: postProcessor->blocklist)
        {
            if (group==0 || block.InGroup == group)
            {
                block.ToggleSelect();
            }
            postProcessor->bHasMask = false;
        }
    }
}

FemmAPI::CircuitProperties FemmAPI::mo_getcircuitproperties(const char* circuit) const
{
    if (!postProcessor)
        return {};

    const auto searchResult = doc->circuitMap.find(circuit);
    if (searchResult == doc->circuitMap.end())
        return {};
    
    const int idx = searchResult->second;

    const CComplex amps = postProcessor->circproplist[idx].Amps;
    const CComplex volts = postProcessor->GetVoltageDrop(idx);
    const CComplex fluxLinkage = postProcessor->GetFluxLinkage(idx);

    return { amps, volts, fluxLinkage };
}

CComplex FemmAPI::mo_blockintegral(int type)
{
    bool hasSelectedBlocks = false;
    for (const auto &block: postProcessor->blocklist )
    {
        if (block.IsSelected)
        {
            hasSelectedBlocks = true;
            break;
        }
    }

    if (!hasSelectedBlocks)
        return 0;

    if ((type>=18) && (type<=23))
    {
        postProcessor->MakeMask();
    }

    return postProcessor->BlockIntegral(type);
}
