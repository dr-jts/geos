/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2020 Martin Davis
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation.
 * See the COPYING file for more information.
 *
 **********************************************************************/

#include <geos/profiler.h>
#include <geos_c.h>

#include <geos/geom/GeometryFactory.h>
#include <geos/operation/valid/MakeValid.h>
#include <geos/io/WKTReader.h>
#include <geos/io/WKBReader.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "WKTStreamReader.h"
#include "WKBStreamReader.h"
#include "GeosOp.h"
#include "cxxopts.hpp"

using namespace geos;
using namespace geos::geom;

std::string const GeosOp::opNames[] = {
    "area",
    "boundary",
    "buffer D",
    "centroid",
    "convexHull",
    "envelope",
    "interiorPoint",
    "isValid",
    "length",
    "union",
};


void showHelp() {
    std::cout << "geosop executes GEOS geometry operations on inputs." << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: geosop [wktfile] opname args..." << std::endl;
}

int main(int argc, char** argv) {
    GeosOpArgs cmdArgs;

    cxxopts::Options options("geosop", "Executes GEOS geometry operations");
    options.add_options()
        ("a", "source for A geometries", cxxopts::value<std::string>( cmdArgs.srcA ))
        ("alimit", "limit on A geometries", cxxopts::value<int>( cmdArgs.limitA ))
        ("c,collect", "Collect input into single geometry", cxxopts::value<bool>( cmdArgs.isCollect ))
        ("f,format", "Output format", cxxopts::value<std::string>( ))
        ("h,help", "Print help")
        ("t,time", "Print execution time", cxxopts::value<bool>( cmdArgs.isShowTime ) )
        ("v,verbose", "Verbose output", cxxopts::value<bool>( cmdArgs.isVerbose )->default_value("false"))

        ("opName", "Operation name", cxxopts::value<std::string>()->default_value("no-op"))
        ("opArgs", "Operation arguments ", cxxopts::value<std::vector<std::string>>())
    ;

    options.parse_positional({"opName", "opArgs"});
    auto result = options.parse(argc, argv);

    if (argc == 0 || result.count("help")) {
        std::cout << options.help() << std::endl;
        //showHelp();
        if (result.count("help")) {
            std::cout << "Operations:" << std::endl;
            for (auto opName : GeosOp::opNames) {
               std::cout << "  " << opName << std::endl;
            }
        }
        return 0;
    }

    if (result.count("format")) {
        auto fmt = result["format"].as<std::string>();
        if (fmt == "txt" || fmt == "wkt" ) {
            cmdArgs.format = GeosOpArgs::fmtText;
        }
        else if (fmt == "wkb") {
            cmdArgs.format = GeosOpArgs::fmtWKB;
        }
        else {
            std::cerr << "Invalid format value: " << fmt << std::endl;
            exit(1);
        }
    }

    if (result.count("opName")) {
        cmdArgs.opName = result["opName"].as<std::string>();
    }
    //--- parse positional op arg (only one supported for now)
    if (result.count("opArgs"))
    {
        auto& v = result["opArgs"].as<std::vector<std::string>>();
        if (v.size() >= 1) {
            auto val = v[0];
            cmdArgs.opArg1 = std::stod(val);
        }
    }

    GeosOp geosop(cmdArgs);
    geosop.run();
}

GeosOp::GeosOp(GeosOpArgs& arg)
    : args(arg)
{
}

GeosOp::~GeosOp() {
}

std::string timeFormatted(int n)
{
    auto fmt = std::to_string(n);
    int insertPosition = static_cast<int>(fmt.length()) - 3;
    while (insertPosition > 0) {
        fmt.insert(insertPosition, ",");
        insertPosition-=3;
    }
    return fmt + " usec";
}

static bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

static bool endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

std::vector<std::unique_ptr<Geometry>>
collect( std::vector<std::unique_ptr<Geometry>>& geoms ) {

    std::vector<Geometry*> geomsCopy;
    for (int i = 0; i < geoms.size(); i++) {
        auto gCopy = geoms[i]->clone();
        geomsCopy.push_back( gCopy );
    }
		auto gf = GeometryFactory::create();
    auto gc = gf->createGeometryCollection( geomsCopy );

    std::vector<std::unique_ptr<Geometry>> geomsColl;
    geomsColl.push_back( std::unique_ptr<Geometry>(gc) );
    return geomsColl;
}

bool isWKTLiteral(std::string s) {
    // TODO: fix this to handle e.g. POLYGON EMPTY

    // assume if string contains a ( it is WKT
    int numLParen = std::count(s.begin(), s.end(), '(');
    return numLParen > 0;
}

bool isWKBLiteral(std::string s) {
    // assume WKB if only chars are [0-9] and [a-fA-F]
    const std::string hexChars = "0123456789abcdefABCDEF";
    return s.find_first_not_of(hexChars) == std::string::npos;
}

std::vector<std::unique_ptr<Geometry>>
readWKTFile(std::istream& in, int limit) {

    WKTStreamReader rdr( in );
    std::vector<std::unique_ptr<Geometry>> geoms;
    int count = 0;
    while (limit < 0 || count < limit) {
        auto geom = rdr.next();
        if (geom == nullptr)
            break;
        geoms.push_back( std::unique_ptr<Geometry>(geom) );
        count++;
    }
    return geoms;
}

std::vector<std::unique_ptr<Geometry>>
readWKTFile(std::string src, int limit) {
    if (src == "-" || src == "-.wkt" || src == "stdin" || src == "stdin.wkt") {
        return readWKTFile( std::cin, limit );
    }
    std::ifstream f( src );
    auto geoms = readWKTFile( f, limit );
    f.close();
    return geoms;
}

std::vector<std::unique_ptr<Geometry>>
readWKBFile(std::istream& in, int limit) {
    WKBStreamReader rdr( in );
    std::vector<std::unique_ptr<Geometry>> geoms;
    int count = 0;
    while (limit < 0 || count < limit) {
        auto geom = rdr.next();
        if (geom == nullptr)
            break;
        geoms.push_back( std::unique_ptr<Geometry>(geom) );
        count++;
    }
    return geoms;
}

std::vector<std::unique_ptr<Geometry>>
readWKBFile(std::string src, int limit) {
    if (src == "-.wkb" || "stdin.wkb" ) {
        return readWKBFile( std::cin, limit );
    }
    std::ifstream f( src );
    auto geoms = readWKBFile( f, limit );
    f.close();
    return geoms;
}

std::vector<std::unique_ptr<Geometry>>
GeosOp::readInput(std::string name, std::string src, int limit) {
    std::vector<std::unique_ptr<Geometry>> geoms;
    std::string srcDesc;
    if ( isWKTLiteral(src) ) {
        srcDesc = ": WKT literal";

        geos::io::WKTReader rdr;
        auto geom = rdr.read( src );
        geoms.push_back( std::unique_ptr<Geometry>(geom) );
    }
    else if ( isWKBLiteral(src) ) {
        srcDesc = "WKB literal";

        geos::io::WKBReader rdr;
        std::istringstream hex(src);
        auto geom = rdr.readHEX( hex );
        geoms.push_back( std::unique_ptr<Geometry>(geom) );
    }
    else if (endsWith(src, ".wkb")) {
        srcDesc = "WKB file " + src;
        geoms = readWKBFile( src, limit );
    }
    else {
        srcDesc = "WKT file " + src;
        geoms = readWKTFile( src, limit );
    }
    if (args.isVerbose) {
        std::cout << "Input " << name << ": " << srcDesc << std::endl;
    }
    return geoms;
}

std::string geomStats(int geomCount, int geomVertices) {
    return std::to_string(geomCount) + " geometries, "
        + std::to_string(geomVertices) + " vertices";

}
std::string summaryStats(std::vector<std::unique_ptr<Geometry>>& geoms) {
    int geomCount = 0;
    int geomPts = 0;
    for (const  auto& geom : geoms) {
        geomCount++;
        geomPts += geom->getNumPoints();
    }
    return geomStats(geomCount, geomPts);
}

void GeosOp::run() {

    initGEOS(nullptr, nullptr);

    geos::util::Profile sw("read");
    sw.start();
    auto geomsLoad = readInput( "A", args.srcA, args.limitA );
    statsA = summaryStats(geomsLoad);
    sw.stop();
    if (args.isVerbose) {
        std::cout << "Read " << statsA
        << "  -- " << timeFormatted( sw.getTot() )
        << std::endl;
    }

    //--- collect input into single geometry collection if specified
    if (args.isCollect && geomsLoad.size() > 1) {
        geomA = collect( geomsLoad );
    }
    else {
        geomA = std::move(geomsLoad);
    }

    execute();
}

void GeosOp::execute() {
    std::string op = args.opName;

    //std::cout << "DEBUG Format: " << args.format << std::endl;

    geos::util::Profile sw( op );
    sw.start();

    for (const auto& geom : geomA) {
        opCount++;
        Result* result = executeOp(op, geom);

        output(result);
        delete result;
    }

    sw.stop();
    if (args.isShowTime || args.isVerbose) {
        std::cout
            << "Processed " <<  statsA
            << "  -- " << timeFormatted( sw.getTot() )
            << std::endl;
    }
}

void GeosOp::output(Result* result) {
    //---- print result if format specified
    if (args.format == GeosOpArgs::fmtNone )
        return;

    if (result->isGeometry() && args.format == GeosOpArgs::fmtWKB ) {
        std::cout << *(result->valGeom) << std::endl;
    }
    else {
        // output as text/WKT
        std::cout << result->toString() << std::endl;
    }
}

//TODO: reify operations into GeomOp classes (or instances with a function pointer?)
// This allows pre-checking op existence, and providing metadata about ops (name, description)

Result* GeosOp::executeOp(std::string op, const std::unique_ptr<Geometry>& geom) {

    geos::util::Profile sw( op );
    sw.start();

    Result* result;
    if (op == "" || op == "no-op") {
        result = new Result( geom->clone() );
    } else if (op == "area") {
        result = new Result( geom->getArea() );
    } else if (op == "boundary") {
        result = new Result( geom->getBoundary() );
    } else if (op == "buffer") {
        result = new Result( geom->buffer( args.opArg1 ) );
    } else if (op == "convexHull") {
        result = new Result( geom->convexHull() );
    } else if (op == "centroid") {
        result = new Result( geom->getCentroid() );
    } else if (op == "envelope") {
        result = new Result( geom->getEnvelope() );
    } else if (op == "interiorPoint") {
        result = new Result( geom->getInteriorPoint() );
    } else if (op == "isValid") {
         result = new Result( geom->isValid() );
    } else if (op == "length") {
        result = new Result( geom->getLength() );
     } else if (op == "union") {
        result = new Result( geom->Union() );
    } else {
        std::cerr << "Unknown operation: " << op << std::endl;
        exit(1);
    }
    if (args.isVerbose) {
        sw.stop();
        std::cout
            << "[ " << opCount << "] "
            << args.opName << ": "
            << geom->getGeometryType() << "( " << geom->getNumPoints() << " )"
            << " -> " << result->metadata()
            << "  --  " << timeFormatted( sw.getTot() )
            << std::endl;
    }

    return result;
}

//===============================================

Result::Result(bool val)
{
    valBool = val;
    typeCode = typeBool;
}

Result::Result(int  val)
{
    valInt = val;
    typeCode = typeInt;
}

Result::Result(double val)
{
    valDouble = val;
    typeCode = typeDouble;
}

Result::Result(std::unique_ptr<geom::Geometry> val)
{
    valGeom = std::move(val);
    typeCode = typeGeometry;
}

Result::~Result()
{
}

bool
Result::isGeometry() {
    return typeCode == typeGeometry;
}

std::string
Result::toString() {
    std::stringstream converter;
    switch (typeCode) {
    case typeBool:
        converter << std::boolalpha << valBool;
        return converter.str();

    case typeInt:
        converter << valInt;
        return converter.str();

    case typeDouble:
        converter << valDouble;
        return converter.str();

    case typeGeometry:
        return valGeom->toString();
    }
}

std::string
Result::metadata() {
    std::stringstream converter;
    switch (typeCode) {
    case typeBool:
        return "bool";

    case typeInt:
        return "int";

    case typeDouble:
        return "double";

    case typeGeometry:
        return valGeom->getGeometryType() + "( " + std::to_string( valGeom->getNumPoints() ) + " )";
    }
}
