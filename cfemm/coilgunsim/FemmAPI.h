#pragma once

#include <femmcomplex.h>
#include <FemmProblem.h>

#ifndef FEMM_CAPI_H
#define FEMM_CAPI_H

class FPProc;

namespace fmesher
{
    class FMesher;
}

class FemmAPI
{
public:
    struct CircuitProperties
    {
        CComplex Current;
        CComplex Voltage;
        CComplex FluxLinkage;
    };

    struct BoundingBox
    {
        double x[2];
        double y[2];
    };
    
private:
    std::shared_ptr<femm::FemmProblem> doc;
    std::shared_ptr<fmesher::FMesher> mesher;
    std::shared_ptr<FPProc> postProcessor;
    
public:
    void femm_init(const char* file);
    void femm_save(const char* file);
    void femm_close();

    void smartmesh(bool enable);
    void mi_probdef(int frequency,
        femm::LengthUnit lengthUnits,
        femm::ProblemType problemType,
        double precision,
        double depth,
        double min_angle);
    void mi_getmaterial(const char* matname);
    void mi_addmaterial(const char* blockName, double mu_x, double mu_y,
        double H_c = 0.0, double J = 0.0, double Cduct = 0.0, double Lam_d = 0.0, double Theta_hn = 0.0,
        double LamFill = 0.0, int LamType = 0, double Theta_hx = 0.0, double Theta_hy = 0.0, int NStrands = 0,
        double WireD  = 0.0);
    void mi_addnode(double x, double y);
    void mi_addarc(double sx, double sy, double ex, double ey, double angle, double maxseg);
    void mi_drawline(double x1, double y1, double x2, double y2);
    void mi_drawarc(double sx, double sy, double ex, double ey, double angle, double maxseg);
    void mi_selectnode(double x, double y);
    void mi_clearselected();
    void mi_setnodeprop(int group_id, const char* boundary_marker_name = nullptr);
    void mi_modifymaterial(const char* matname, int prop_id, void* value);
    void mi_addcircprop(const char* circuit_name, int amps, int circuit_type);
    void mi_addsegment(double sx, double sy, double ex, double ey);
    void mi_addblocklabel(double x, double y);
    void mi_selectlabel(double x, double y);
    void mi_setblockprop(const char* blocktype, bool automesh, double meshsize, const char* incircuit, double magdirection, int group, int turns);
    void mi_movetranslate(double x, double y);
    void mi_selectgroup(int group);
    void mi_modifycircprop(const char* circuit, int prop_id, void* value);
    void mi_saveas(const char* filename);
    int mi_analyze();
    int mi_loadsolution();
    void mo_groupselectblock(int group);
    CircuitProperties mo_getcircuitproperties(const char* circuit) const;
    CComplex mo_blockintegral(int type);

    BoundingBox mi_getboundingbox() const;
    void mi_addboundprop(const char* boundName, double A0, double A1, double A2, double phi, double Mu, double Sig, double c0, double c1, int format);
    void mi_selectarcsegment(double mx, double my);
    void mi_setarcsegmentprop(double maxsegdeg, const char* boundprop, bool hide, int group);
    void mi_makeABC(int numLayers, double radius, int bctype);
};
#endif // FEMM_CAPI_H
