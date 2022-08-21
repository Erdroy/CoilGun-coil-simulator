#pragma once

#include <femmcomplex.h>
#include "FemmAPI.h"

class FemmExtensions
{
public:
    static void AddLine(FemmAPI& api, const double x0, const double y0, const double x1, const double y1, const int group = 0)
    {
        api.mi_addnode(x0, y0);
        api.mi_selectnode(x0, y0);
        api.mi_setnodeprop(group);
        
        api.mi_addnode(x1, y1);
        api.mi_selectnode(x1, y1);
        api.mi_setnodeprop(group);

        api.mi_addsegment(x0, y0, x1, y1);
        
        api.mi_clearselected();
    }

    static void AddBlock(FemmAPI& api, const double x, const double y, const char* name, const char* circuit, const int group, const int turns)
    {
        api.mi_addblocklabel(x, y);
        api.mi_selectlabel(x, y);
         
        api.mi_setblockprop(name, true, 0, circuit, 0, group, turns);
    }

    static void MoveGroup(FemmAPI& api, const double x, const double y, const int group)
    {
        api.mi_clearselected();
        api.mi_selectgroup(group);
        api.mi_movetranslate(x, y);
    }

    static void SetCircuitCurrent(FemmAPI& api, const char* circuit, int current)
    {
        api.mi_modifycircprop(circuit, 1, &current);
    }

    static void Analyze(FemmAPI& api, const char* fileName)
    {
        api.mi_saveas(fileName);
        api.mi_analyze();
        api.mi_loadsolution();
    }

    static CComplex IntegrateBlockForce(FemmAPI& api, const char* circuit, const int current, const int block, const char* fileName)
    {
        api.mi_clearselected();
        
        SetCircuitCurrent(api, circuit, current);
        Analyze(api, fileName);
        api.mo_groupselectblock(block);
        return api.mo_blockintegral(19);
    }

    static CComplex IntegrateInductance(FemmAPI& api, const char* circuit, const int current, const char* fileName)
    {
        api.mi_clearselected();
        
        SetCircuitCurrent(api, circuit, current);
        Analyze(api, fileName);
        api.mo_groupselectblock(0);
        return  (api.mo_blockintegral(2) * 2.0) / (current * current) * 1E6;
    }
};
