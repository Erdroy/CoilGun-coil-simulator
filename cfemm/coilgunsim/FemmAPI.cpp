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

void FemmAPI::mi_addmaterial(const char* blockName, double mu_x, double mu_y, double H_c, double J, double Cduct, double Lam_d, double Theta_hn, double LamFill, int LamType, double Theta_hx, double Theta_hy, int NStrands, double WireD)
{
    std::unique_ptr<femm::CMSolverMaterialProp> m = std::make_unique<femm::CMSolverMaterialProp>();
    
    m->BlockName = blockName;
    m->mu_x=mu_x;
    m->mu_y=mu_y;
    m->H_c=H_c;
    m->J=J;
    m->Cduct=Cduct;
    m->Lam_d=Lam_d;
    m->Theta_hn=Theta_hn;

    m->LamFill=LamFill;
    if (m->LamFill<=0) m->LamFill=1;
    if (m->LamFill>1) m->LamFill=1;
    
    m->LamType=LamType;
    if (m->LamType<0) m->LamType=0;
    
    m->Theta_hx=Theta_hx;
    m->Theta_hy=Theta_hy;
    if(abs(Theta_hx) < 0.0001 && abs(Theta_hy) < 0.0001)
    {
        m->Theta_hx=m->Theta_hn;
        m->Theta_hy=m->Theta_hn;
    }
    
    m->NStrands=NStrands;
    m->WireD=WireD;

    doc->blockproplist.push_back(std::move(m));
    doc->updateBlockMap();
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

void FemmAPI::mi_drawline(double x1, double y1, double x2, double y2)
{
    mi_addnode(x1,y1);
    mi_addnode(x2,y2);
    mi_addsegment(x1,y1,x2,y2);
}

void FemmAPI::mi_drawarc(double sx, double sy, double ex, double ey, double angle, double maxseg)
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



FemmAPI::BoundingBox FemmAPI::mi_getboundingbox() const
{
    double x[2],y[2];
    if (doc->getBoundingBox(x,y))
    {
        FemmAPI::BoundingBox box{};
        box.x[0] = x[0];
        box.x[1] = x[1];
        box.y[0] = y[0];
        box.y[1] = y[1];
        return box;
    }
    return {};
}

void FemmAPI::mi_addboundprop(const char* boundName, double A0, double A1, double A2, double phi, double Mu, double Sig,
    double c0, double c1, int format)
{
    std::unique_ptr<femm::CMBoundaryProp> m = std::make_unique<femm::CMBoundaryProp>();
    m->BdryName = boundName;
    m->A0 = A0;
    m->A1 = A1;
    m->A2 = A2;
    m->phi = phi;
    m->Mu = Mu;
    m->Sig = Sig;
    m->c0 = c0;
    m->c1 = c1;
    m->BdryFormat = format;
    
    doc->lineproplist.push_back(std::move(m));
    doc->updateLineMap();
}

void FemmAPI::mi_selectarcsegment(double mx, double my)
{
    if (doc->arclist.empty())
        return 0;

    int node = doc->closestArcSegment(mx,my);
    doc->arclist[node]->ToggleSelect();
}

struct BoundsData
{
    double v[12];
};

BoundsData uAx0[12] = {{10.06344410876133}, 
				{0.18870625462846807, 39.997500411566335}, 
				{4.374794457961015, 0.07511795918154143, 85.85566438727763}, 
				{0.28509610532711227, 12.493269046515916, 0.051874008856059424, 147.65207370446487}, 
				{3.0607674892527865, 0.11237818729856126, 22.026456394011774, 0.042316694272791736, 225.43020812455183}, 
				{0.3748159380792134, 8.172423914252851, 0.07400271964825651, 33.01915762825664, 0.037123381446484645, 319.2335657378115}, 
				{2.4089242666740582, 0.14929929105743517, 13.817592213008757, 0.05821722458526954, 45.655954531895645, 0.03386729523177279, 429.0464280099345}, 
				{0.4577193805314985, 6.152383059834071, 0.09519535460494778, 19.79502033933107, 0.04958213970480729, 60.00201778458946, 0.031639182082296755, 554.8893696637082}, 
				{2.0187133623041955, 0.1872419300750821, 10.322990689266506, 0.07322869764123259, 26.290072452745143, 0.0441395144643266, 76.08450824714102, 0.030019516604331355, 696.7499046385117}, 
				{0.5332409067449392, 4.931200653946399, 0.11642450959044752, 14.522111149156393, 0.0612363797098224, 33.37497351570902, 0.04039926157266519, 93.91981759395829, 0.028790057140879656, 854.6396765772441},
				{1.7619160942694887, 0.2265234822569949, 8.305694616317721, 0.08811132253969434, 18.911084918162924, 0.053672710920365976, 41.08415574149624, 0.03767250616088464, 113.51592772683063, 0.02782511889284403, 1028.551339640375},
				{0.6010640087773658, 4.100907065931257, 0.13795163994918203, 11.604922157909524, 0.07274062063786199, 23.5602014704398, 0.048468764953025635, 49.437385787628045, 0.035597711447375925, 134.87895858524496, 0.02704793878013551, 1218.4906640339432}};

BoundsData u2D0[12] = {{10.523809523809524}, 
				{0.14531359011819187, 32.14290974079077}, 
				{5.14939698507243, 0.053268993904189134, 54.10636468483519}, 
				{0.2419609512179885, 13.681253040072905, 0.03301443572330777, 76.07498920312688}, 
				{3.4661091134486472, 0.09209257399023746, 21.797339506447933, 0.02406322785653838, 98.05449854377491}, 
				{0.3336928992521675, 9.023667719158183, 0.05789695118959376, 29.573300006204246, 0.018966499540708268, 120.04039743815603}, 
				{2.649899689611667, 0.12953316809300527, 14.398967120915913, 0.04288257435633827, 37.197476986533054, 0.01566424606410637, 142.03015659052124}, 
				{0.4194212714207981, 6.739646495379991, 0.0807327554079406, 19.40925290947297, 0.034255287061558344, 44.73740232258131, 0.013346899201001426, 164.02239651319712}, 
				{2.1751715650719827, 0.16743379223593088, 10.885675868734184, 0.05987838745046158, 24.237375183864483, 0.028600398122246736, 52.225223074983184, 0.0116295354236669, 186.01632084646832}, 
				{0.4982294821100038, 5.3544134782776664, 0.10291820111116401, 14.696545405270147, 0.048003097591685244, 28.95632726953973, 0.024587012459820066, 59.67849241525681, 0.010305182059498453, 208.01143556937637},
				{1.8696806205952006, 0.2063934470334104, 8.775890039744148, 0.07607619682670601, 18.328256642424236, 0.04023921548166901, 33.60353647639198, 0.021581843440607016, 67.10765452494896, 0.009252450549701629, 230.0074236348898},
				{0.5695628007801712, 4.41799489542221, 0.12499429930905502, 11.902080658968998, 0.06098009575496433, 21.847988015751035, 0.03472775671050834, 38.20087849238854, 0.01924286302232932, 74.51932204012905, 0.00839537165467591, 252.00407064779833}};

BoundsData uAx1[12] = {{0.09090909090909091}, 
				{10.050515900211503, 0.04349612665401649}, 
				{0.16015012096541084, 31.212316587655547, 0.035617897941640685}, 
				{5.075223589959199, 0.06747975164123507, 58.83076738028796, 0.032496412704901965}, 
				{0.24983741484804536, 13.959519296714365, 0.04850942160789329, 93.62679411332631, 0.030834135426565015}, 
				{3.438673996192938, 0.10007632616355028, 23.68319910391683, 0.040588823403447945, 135.81886600712073, 0.029800226566381535}, 
				{0.33861114976053774, 9.11830816329029, 0.06748841981310061, 34.53152988922068, 0.03624009732510784, 185.4917052298402, 0.02909733301094425}, 
				{2.6364338943965455, 0.13486508168984512, 15.097445398169498, 0.05395605788825772, 46.75319306175651, 0.03349209595847733, 242.6651438041285, 0.028587355212278894}, 
				{0.4227311219273812, 6.773013674320017, 0.08745530894064872, 21.301429520550652, 0.04651420711694375, 60.44151623434645, 0.03160001126426384, 307.3631853353253, 0.028201318378415573}, 
				{2.1675591338727647, 0.171316009911807, 11.198891625678383, 0.0680375716016307, 27.948228063433724, 0.041804487302954434, 75.63799976250523, 0.030217675069207188, 379.586805599126, 0.027898435150547626},
				{0.5005548322806631, 5.365891158623955, 0.1079474936583888, 15.594898520264833, 0.05739172309462815, 35.12584682017268, 0.03855673261275498, 92.36682653198929, 0.029164062639383995, 459.3468429136272, 0.027654812310257123},
				{1.864938491614021, 0.20938145992414733, 8.933546101982833, 0.08240945574778373, 20.142380617987286, 0.05066029544301638, 42.87550155678245, 0.036182409831124134, 110.63472923382548, 0.0283338992149914, 546.6150102543896, 0.027453323354323905}};

BoundsData u2D1[12] = {{0.09502262443438914}, 
				{6.88166880459455, 0.031111060201589526}, 
				{0.19419749592018187, 18.77264665066931, 0.018482113995736214},
				{4.13289828365353, 0.07309272016758717, 30.28978015498877, 0.013144924639159823}, 
				{0.28850793996067764, 10.858638831248303, 0.04587715852680953, 41.55718451247941, 0.010198410219328852}, 
				{2.9967673937356176, 0.1108196834283798, 17.27206665382645, 0.0338142851758244, 52.724541914214335, 0.008330528899783033}, 
				{0.37737277525645235, 7.720030434644211, 0.0694494258951436, 23.31949550539958, 0.026883543752277105, 63.83965087550882, 0.0070407582729502555}, 
				{2.384237702546398, 0.14837573304995255, 12.386546277094604, 0.05152181760881001, 29.192574288770942, 0.022352660997929614, 74.923770274119, 0.006096728349565376}, 
				{0.45973385090978197, 5.972510009155741, 0.09186384125878651, 16.700516539917132, 0.04125859307841753, 34.96454824599637, 0.019147835875477914, 85.98795769304176, 0.005375872372109578}, 
				{2.007107238545973, 0.1867618188354154, 9.71645432200938, 0.06804320147518493, 20.83198897925314, 0.0345347664671527, 40.67187917337226, 0.01675645545872318, 97.03855732255441, 0.004807428001555607},
				{0.5348507060428623, 4.845115067234294, 0.11394855626850516, 13.144715978348659, 0.05456056293348216, 24.85137913425649, 0.029758772583431677, 46.33524484374972, 0.014901429756097718, 108.07947522964582, 0.004347685758123113},
				{1.755732640246568, 0.22634702476369292, 8.00036486086015, 0.08401892313227052, 16.39879353450492, 0.04577080504067754, 28.795410205618264, 0.026177408464552673, 51.96731893999375, 0.013419338402750022, 119.11324967287628, 0.003968189868637491}};
				

void FemmAPI::mi_makeABC(int enn, double arr, double ex, double wye, int bc)
{
    double d, z, r, x, y, R;
    int flag = 1; // axisim only
	
    const auto bounds = mi_getboundingbox();
    double x0 = bounds.x[0];
    double x1 = bounds.x[1];
    double y0 = bounds.y[0];
    double y1 = bounds.y[1];
    
    // unpack parameters;
    if (enn>12)
        enn=12;
    else if (enn<1)
        enn=1;
	
    int bctype=bc;
	
    if(flag == 0) // 2D planar case
    {
        R = arr;
        x=ex;
        y=wye;
    }
    else //  Axi case
    {
        x=0;
        if (abs(wye) < 0.001)
        {
            y=wye;
            R=arr;
        }
        else if (abs(ex) < 0.001)
        {
            y=ex;
            R=arr;
        }
        else if (abs(arr) < 0.001)
        {
            y=(y0+y1)/2;
            R=arr;
        }
        else
        {
            y=(y0+y1)/2;
            R=(3/2)*abs(x1+I*(y1-y0)/2);
        }
    }
	
    // draw left boundary of interior domain
    if (flag == 0)
    {
        mi_drawarc(x, y + R, x, y - R, 180, 1);
    }
    else
    {
        mi_drawline(0, y-1.1*R, 0, y+1.1*R);
    }
	
    // draw right boundary of interior domain
    mi_drawarc(x, y - R, x, y + R, 180, 1);
	
    d = 0.1*R/(2*enn);
    
    for(int k = 1; k < enn; k ++)
    {
        const auto str_n = std::to_string(k);
        const auto str = std::string("u") + str_n;
        
        r = R*(1.0 + (2.0 * k - 1.0)/(20*k));
        mi_drawarc(x, y - r - d, x, y + r + d, 180, 1);
        z = (r*exp(I*(90/(k+1))*k*PI/180)).Abs();
        mi_addblocklabel(x+Re(z),y+Im(z));
        mi_selectlabel(x+Re(z),y+Im(z));
        mi_setblockprop(str.c_str(), true, 0, "<None>", 0, 0, 1);
        mi_clearselected();
        if(flag == 0)
        {
            if (bctype==0)
                mi_addmaterial(str.c_str(), u2D0[k].v[k], u2D0[k].v[k]);
            else
                mi_addmaterial(str.c_str(), u2D1[k].v[k], u2D1[k].v[k]);
            mi_drawarc(x, y + r + d, x, y - r - d, 180, 1);
        }
        else
        {
            if (bctype==0)
                mi_addmaterial(str.c_str(), uAx0[k].v[k], uAx0[k].v[k]);
            else
                mi_addmaterial(str.c_str(), uAx1[k].v[k], uAx1[k].v[k]);
        }
    }
	
	if (bctype==0)
	{
	    mi_addboundprop("A=0", 0, 0, 0, 0, 0, 0, 0, 0, 0);
	    mi_selectarcsegment(1.1*R+x, y);
	    if(flag == 0)
            mi_selectarcsegment(-1.1*R+x, y);
        mi_setarcsegmentprop(1, "A=0", 0, 0);
	    mi_clearselected();
	}
}
