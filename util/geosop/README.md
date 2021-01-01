# GeosOp User Guide

`geosop` is a CLI (command-line interface) for GEOS.
It can be used to:

* Run GEOS operations on lists of geometries
* Output geometry in various formats (WKT and WKB)
* Time the performance of operations
* Check for memory leaks in operations

## Features

* Read list of geometries from a file
* TBD: Read geometries from stdin
* Read geometry from command-line literal
* Input format is WKT or WKB (TBD)
* Apply a limit and offset (TBD) to the input geometries
* TBD: collect input geometries into a GeometryCollection (for aggregate operations)
* Execute a GEOS operation on each geometry
* TBD: Execute a GEOS operation on each geometry for a list of different arguments
* Output result as text, WKT or WKB
* Time the overall and individual (TBD) performance of each operation


## Examples

**Note: TBD = To Be Developed**

* Print usage instructions

    geosop

* Print usage instructions and list of available operations

    geosop --help

* Compute the area of geometries in a WKT file and output them as text

    geosop -a geoms.wkt --format=txt area

* Compute the centroids of geometries in a WKT file and output them as WKT

    geosop -a geoms.wkt -f wkt centroid

* Compute an operation on a list of geometries and output only geometry metrics and timing

    geosop -v -a geoms.wkt isValid

* Validate geometries from a WKT file, limiting the number of geometries read

    geosop -a geoms.wkt --alimit 100 -f txt isValid

* Compute the buffer of distance 10 of WKT geometries and output as WKT

    geosop -a geoms.wkt -f wkt buffer 10

* TBD: Compute the unary union of a set of WKT geometries and output as WKB

    geosop -a geoms.wkt -collect -f wkt union

* TBD: Compute the union of two geometries in WKT and WKB and output as WKT

    geosop -a some-geom.wkt -b some-other-geom.wkb -f wkt union

* TBD: Compute the buffer of a WKT literal for multiple distances

    geosop -a "MULTIPOINT ( (0 0), (10 10) )" -f wkt buffer 1,2,3,4

* TBD: Compute the buffer of a WKB literal and output as WKT

    geosop -a 000000000140240000000000004024000000000000 -f wkt buffer 10

* TBD: Compute the buffer of a WKT literal and output as WKB, with SRID set to 4326

    geosop -a  "POINT (10 10)" --srid=4326 -f wkb buffer 10

* TBD: Read geometries from stdin and output as WKB

    geosop -a - -f wkb



## Future Ideas

* `--sort [ asc | desc ]` sorts output geometries by value of operation
* `--select [eq | gt |ge | lt | le ] val` selects input geometries by value of operation
* `--limit N` applies limit to output (used with sorting)
