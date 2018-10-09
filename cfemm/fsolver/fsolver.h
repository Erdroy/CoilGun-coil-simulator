/*
   This code is a modified version of an algorithm
   forming part of the software program Finite
   Element Method Magnetics (FEMM), authored by
   David Meeker. The original software code is
   subject to the Aladdin Free Public Licence
   version 8, November 18, 1999. For more information
   on FEMM see www.femm.info. This modified version
   is not endorsed in any way by the original
   authors of FEMM.

   This software has been modified to use the C++
   standard template libraries and remove all Microsoft (TM)
   MFC dependent code to allow easier reuse across
   multiple operating system platforms.

   Date Modified: 2017
   By: Richard Crozier
       Johannes Zarl-Zierl
   Contact:
        richard.crozier@yahoo.co.uk
        johannes.zarl-zierl@jku.at

   Contributions by Johannes Zarl-Zierl were funded by
   Linz Center of Mechatronics GmbH (LCM)
*/

// fsolver.h : interface of the FSolver class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef FSOLVER_H
#define FSOLVER_H

#include <string>
#include <vector>
#include "feasolver.h"
#include "cspars.h"
#include "CBlockLabel.h"
#include "CCircuit.h"
#include "CElement.h"
#include "CMaterialProp.h"
#include "CNode.h"
#include "CPointProp.h"

namespace femm {
class LuaInstance;
}

class FSolver : public FEASolver<
        femm::CMPointProp
        , femm::CMBoundaryProp
        , femm::CMSolverMaterialProp
        , femm::CMCircuit
        , femm::CMBlockLabel
        , femmsolver::CMElement
        , femm::CMAirGapElement
        >
{

// Attributes
public:

    FSolver();
    ~FSolver();

    // General problem attributes
    double Frequency;  ///< \brief Frequency for harmonic problems [Hz]
    double  Relax;

    // mesh information
    femm::CNode *meshnode;
    int NumCircPropsOrig;


// Operations
public:

    LoadMeshErr LoadMesh(bool deleteFiles=true) override;
    /**
     * @brief loadPreviousSolution
     * @return \c true on success, \c false otherwise.
     * \internal
     * ### FEMM reference source
     *  - \femm42{fkn/femmedoccore.cpp,CFemmeDocCore::LoadPrev()}
     * \endinternal
     */
    bool loadPreviousSolution();
    bool LoadProblemFile();
    int Static2D(CBigLinProb &L);
    int WriteStatic2D(CBigLinProb &L);
    int Harmonic2D(CBigComplexLinProb &L);
    int WriteHarmonic2D(CBigComplexLinProb &L);
    int StaticAxisymmetric(CBigLinProb &L);
    int HarmonicAxisymmetric(CBigComplexLinProb &L);
    void GetFillFactor(int lbl);
    double ElmArea(int i);

    virtual bool runSolver(bool verbose=false) override;

private:

    virtual void CleanUp() override;

    /**
     * @brief getPrevAxiB
     * @param k
     * @param B1p
     * @param B2p
     * \internal
     * ### FEMM reference source
     *  - \femm42{fkn/prob4big.cpp,CFemmeDocCore::GetPrevAxiB()}
     * \endinternal
     */
    void getPrevAxiB(int k, double &B1p, double &B2p) const;
    /**
     * @brief getPrev2DB
     * @param k
     * @param B1p
     * @param B2p
     * \internal
     * ### FEMM reference source
     *  - \femm42{fkn/prob2big.cpp,CFemmeDocCore::GetPrev2DB()}
     * \endinternal
     */
    void getPrev2DB(int k, double &B1p, double &B2p) const;

    // override parent class virtual method
    void SortNodes (int* newnum) override;

    bool handleToken(const std::string &token, std::istream &input, std::ostream &err) override;

    femm::LuaInstance *theLua;

    /// Vector containing previous solution for incremental permeability analysis
    double *Aprev;
};

/////////////////////////////////////////////////////////////////////////////

double GetNewMu(double mu,int BHpoints, CComplex *BHdata,double muc,double B);

#endif
