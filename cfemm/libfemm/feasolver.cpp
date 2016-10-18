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

   Date Modified: 2011 - 11 - 10
   By: Richard Crozier
   Contact: richard.crozier@yahoo.co.uk
*/

// fsolver.cpp : implementation of the FSolver class
//

#include "femmcomplex.h"
#include "mesh.h"
#include "spars.h"
#include "fparse.h"
#include "feasolver.h"

#include <ctype.h>
#include <fstream>
#include <ios>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#ifndef _MSC_VER
#define _strnicmp strncasecmp
#endif

using namespace std;
using namespace femm;

/////////////////////////////////////////////////////////////////////////////
// FEASolver construction/destruction

template< class PointPropT
          , class BoundaryPropT
          , class BlockPropT
          , class CircuitPropT
          , class BlockLabelT
          , class NodeT
          >
FEASolver<PointPropT,BoundaryPropT,BlockPropT,CircuitPropT,BlockLabelT,NodeT>
::FEASolver()
{
    Precision = 0;
    LengthUnits = LengthInches;
    ProblemType = PLANAR;
    DoForceMaxMeshArea = false;
    Coords = CART;
    BandWidth = 0;
    NumNodes = 0;
    NumEls = 0;
    NumBlockProps = 0;
    NumPBCs = 0;
    NumLineProps = 0;
    NumPointProps = 0;
    NumCircProps = 0;
    NumBlockLabels = 0;

    extRo = extRi = extZo = 0.0;

    // initialise the warning message box function pointer to
    // point to the PrintWarningMsg function
    WarnMessage = &PrintWarningMsg;
}

template< class PointPropT
          , class BoundaryPropT
          , class BlockPropT
          , class CircuitPropT
          , class BlockLabelT
          , class NodeT
          >
FEASolver<PointPropT,BoundaryPropT,BlockPropT,CircuitPropT,BlockLabelT,NodeT>
::~FEASolver()
{
}
template< class PointPropT
          , class BoundaryPropT
          , class BlockPropT
          , class CircuitPropT
          , class BlockLabelT
          , class NodeT
          >
void FEASolver<PointPropT,BoundaryPropT,BlockPropT,CircuitPropT,BlockLabelT,NodeT>
::CleanUp()
{
    // define some defaults
    ACSolver=0;
    Coords=CART;
    Depth=-1;
    DoForceMaxMeshArea = false;
    Frequency=0.;
    NumBlockProps=0;
    NumCircProps=0;
    NumLineProps=0;
    NumPointProps=0;
    Precision=1.e-08;
    ProblemType=PLANAR;

    nodeproplist.clear();
    lineproplist.clear();
    blockproplist.clear();
    circproplist.clear();
    labellist.clear();
    nodes.clear();
    meshele.clear();
    pbclist.clear();
}

template< class PointPropT
          , class BoundaryPropT
          , class BlockPropT
          , class CircuitPropT
          , class BlockLabelT
          , class NodeT
          >
bool FEASolver<PointPropT,BoundaryPropT,BlockPropT,CircuitPropT,BlockLabelT,NodeT>
::LoadProblemFile(std::string &file)
{
    std::ifstream input;
    std::stringstream msgStream;
    msgStream >> noskipws; // don't discard whitespace from message stream

    input.open(file.c_str(), std::ifstream::in);
    if (!input.is_open())
    {
        printf("Couldn't read from specified .fem file\n");
        return false;
    }

    // define some defaults
    CleanUp();

    // parse the file

    string token;
    while (input.good())
    {
        nextToken(input, &token);

        // Depth for 2D planar problems;
        if( token == "[depth]")
        {
            expectChar(input, '=',msgStream);
            input >> Depth;
            continue;
        }

        // Frequency of the problem
        if( token == "[frequency]")
        {
            expectChar(input, '=',msgStream);
            input >> Frequency;
            continue;
        }

        // Precision
        if( token == "[precision]")
        {
            expectChar(input, '=', msgStream);
            input >> Precision;
            continue;
        }

        // AC Solver Type
        if( token == "[acsolver]")
        {
            expectChar(input, '=', msgStream);
            input >> ACSolver;
            continue;
        }

        // Option to force use of default max mesh, overriding
        // user choice
        if( token == "[forcemaxmesh]")
        {
            expectChar(input, '=', msgStream);
            input >> DoForceMaxMeshArea;
            continue;
        }

        // Units of length used by the problem
        if( token == "[lengthunits]")
        {
            expectChar(input, '=', msgStream);
            nextToken(input, &token);

            if( token == "inches" ) LengthUnits=LengthInches;
            else if( token == "millimeters" ) LengthUnits=LengthMillimeters;
            else if( token == "centimeters" ) LengthUnits=LengthCentimeters;
            else if( token == "mils" ) LengthUnits=LengthMils;
            else if( token == "microns" ) LengthUnits=LengthMicrometers;
            else if( token == "meters" ) LengthUnits=LengthMeters;
            continue;
        }

        // Problem Type (planar or axisymmetric)
        if( token == "[problemtype]" )
        {
            expectChar(input, '=', msgStream);
            nextToken(input, &token);

            if( token == "planar" ) ProblemType=PLANAR;
            if( token == "axisymmetric" ) ProblemType=AXISYMMETRIC;
            continue;
        }

        // Coordinates (cartesian or polar)
        if( token == "[coordinates]" )
        {
            expectChar(input, '=', msgStream);
            nextToken(input, &token);

            if ( token == "cartesian" ) Coords=CART;
            if ( token == "polar" ) Coords=POLAR;
            continue;
        }

        // properties for axisymmetric external region
        if( token == "[extzo]" )
        {
            expectChar(input, '=', msgStream);
            input >> extZo;
            continue;
        }

        if( token == "[extro]" )
        {
            expectChar(input, '=', msgStream);
            input >> extRo;
            continue;
        }

        if( token == "[extri]" )
        {
            expectChar(input, '=', msgStream);
            input >> extRi;
            continue;
        }

        if( token == "[prevsoln]" )
        {
            expectChar(input, '=', msgStream);
            ParseString(input,&previousSolutionFile);
            continue;
        }

        if( token == "[dt]" )
        {
            expectChar(input, '=', msgStream);
            input >> dT;
            continue;
        }

        // Point Properties
        if( token == "[pointprops]" )
        {
            int k;
            expectChar(input, '=', msgStream);
            input >> k;
            if (k>0) nodeproplist.reserve(k);
            while (input.good() && NumPointProps < k)
            {
                PointPropT next = PointPropT::fromStream(input, msgStream);
                nodeproplist.push_back(next);
                NumPointProps++;
            }
            // message will be printed after parsing is done
            if (NumPointProps != k)
            {
                msgStream << "\nExpected "<<k<<" PointProps, but got " << NumPointProps;
                break; // stop parsing
            }
            continue;
        }


        // Boundary Properties;
        if( token == "[bdryprops]" )
        {
            expectChar(input, '=', msgStream);
            int k;
            input >> k;
            if (k>0) lineproplist.reserve(k);

            while (input.good() && NumLineProps < k)
            {
                BoundaryPropT next = BoundaryPropT::fromStream(input, msgStream);
                lineproplist.push_back(next);
                NumLineProps++;
            }
            // message will be printed after parsing is done
            if (NumLineProps != k)
            {
                msgStream << "\nExpected "<<k<<" BoundaryProps, but got " << NumLineProps;
                break; // stop parsing
            }
            continue;
        }


        // Block Properties;
        if( token == "[blockprops]" )
        {
            expectChar(input, '=', msgStream);
            int k;
            input >> k;
            if (k>0) blockproplist.reserve(k);

            while (input.good() && NumBlockProps < k)
            {
                BlockPropT next = BlockPropT::fromStream(input, msgStream);
                blockproplist.push_back(next);
                NumBlockProps++;
            }
            // message will be printed after parsing is done
            if (NumBlockProps != k)
            {
                msgStream << "\nExpected "<<k<<" BlockProps, but got " << NumBlockProps;
                break; // stop parsing
            }
            continue;
        }

        // Circuit Properties
        if( token == "[circuitprops]" || token == "[conductorprops]")
        {
            expectChar(input, '=', msgStream);
            int k;
            input >> k;
            if(k>0) circproplist.reserve(k);

            while (input.good() && NumCircProps < k)
            {
                CircuitPropT next = CircuitPropT::fromStream(input, msgStream);
                circproplist.push_back(next);
                NumCircProps++;
            }
            // message will be printed after parsing is done
            if (NumCircProps != k)
            {
                msgStream << "\nExpected "<<k<<" CircuitProps, but got " << NumCircProps;
                break; // stop parsing
            }
            continue;
        }


        // read in regional attributes
        if(token == "[numblocklabels]" )
        {
            expectChar(input, '=', msgStream);
            int k;
            input >> k;
            if (k>0) labellist.reserve(k);
            NumBlockLabels=k;
            for(int i=0; i<k; i++)
            {
                BlockLabelT blk = BlockLabelT::fromStream(input,msgStream);

                labellist.push_back(blk);
            }
            continue;
        }
        // fall-through; token was not used
        // -> ignore rest of line
        //char s[1024];
        //input.getline(s,1024);
        //std::cerr << "**unused: " <<token << " " << s << std::endl;
    }
    if (input.bad())
    {
        string msg = "Parse error while reading input file " + PathName + ".fem!\n";
        msg += "Last token was: " + token +"\n";
        WarnMessage(msg.c_str());
        return false;
    }
    if (!msgStream.str().empty())
    {
        msgStream << "\n";
        WarnMessage(msgStream.str().c_str());
    }

    return true;
}



