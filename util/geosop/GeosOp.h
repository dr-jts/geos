/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2005 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation.
 * See the COPYING file for more information.
 *
 **********************************************************************/

#ifndef GEOS_GEOSOP_H
#define GEOS_GEOSOP_H

#include <geos/geom/GeometryFactory.h>
#include <geos/geom/PrecisionModel.h>

using namespace geos;
using namespace geos::geom;

class Result {
public:
    bool valBool;
    int valInt;
    double valDouble;
    std::unique_ptr<Geometry> valGeom;

    Result(bool val);
    Result(int val);
    Result(double val);
    Result(Geometry * val);
		Result(std::unique_ptr<Geometry> val);
    ~Result();

    bool isGeometry();
    std::string metadata();
    std::string toString();

private:
    enum {
        typeBool = 1, typeInt, typeDouble, typeGeometry
    } typeCode;
};

class GeosOpArgs {

public:
    enum {
        fmtNone, fmtText, fmtWKB
    } format = fmtNone;

    bool isShowTime = false;
    bool isVerbose = false;

    //std::string format;

    std::string srcA;
    int limitA = -1;
    bool isCollect = true;

    std::string opName;
    double opArg1 = 0.0;
    //std::string opArg2;
};

class GeosOp {

public:
    static std::string const opNames[];

    GeosOp(GeosOpArgs& args);
    ~GeosOp();
    void run();

private:

    GeosOpArgs& args;

    int opCount = 0;

    std::vector<std::unique_ptr<Geometry>> geomA;
    std::string statsA;

    std::vector<std::unique_ptr<Geometry>> readInput(std::string name, std::string src, int limit);
    void execute();
    Result* executeOp(std::string op, const  std::unique_ptr<Geometry>& geom);
    void output(Result* result);
    void log(std::string s);
};

#endif // GEOS_GEOSOP_H
