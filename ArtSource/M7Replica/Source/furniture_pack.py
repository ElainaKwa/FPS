"""Separately interchangeable M7 exterior furniture from the official side view.

This file builds visual art meshes only.  Units are metres, +X faces the muzzle,
and every object is authored in the same assembly coordinates as the receiver.
The reference pixel mapping is supplied by ``api['p']``.  Exporters subsequently
move the asset geometry to its mount origin; roots here intentionally stay zero.
"""
import math


ASSET_NAMES = (
    "SM_M7_Stock_Default", "SM_M7_Stock_Skeleton",
    "SM_M7_Mag_20", "SM_M7_Mag_Extended",
    "SM_M7_Grip_Default", "SM_M7_Grip_D2",
)


def build_furniture_pack(api):
    """Create six complete independent furniture assets, using a dict API."""
    required = ("box", "cyl", "tube", "mesh", "profile", "p",
                "profile_px", "begin_asset")
    missing = [key for key in required if key not in api]
    if missing:
        raise KeyError("Missing model helpers: " + ", ".join(missing))
    result = {}
    active = None
    prefix = ""
    names = set()

    def begin(name, category, mount, default=False):
        nonlocal active, prefix
        root = api["begin_asset"](name, category, mount, is_default=default)
        active = {"root": root, "objects": [], "category": category,
                  "mount_key": mount, "is_default": default}
        result[name] = active
        prefix = name.removeprefix("SM_M7_")

    def call(kind, name, *args, **kwargs):
        full_name = prefix + "_" + name
        if full_name in names:
            raise ValueError("Duplicate mesh name: " + full_name)
        names.add(full_name)
        obj = api[kind](full_name, *args, **kwargs)
        active["objects"].append(obj)
        return obj

    def pts(pixels):
        return [api["p"](x, z) for x, z in pixels]

    def xyz(px, py, y=0.0):
        x, z = api["p"](px, py)
        return (x, y, z)

    def prof(name, pixels, width, mat="polymer", bevel=.0007):
        return call("profile_px", name, pixels, width, mat, bevel=bevel)

    def slab(name, pixels, y, thickness, mat="polymer", bevel=.0003):
        poly = pts(pixels)
        n = len(poly)
        verts = [(x, yy, z) for yy in (y-thickness/2, y+thickness/2)
                 for x, z in poly]
        faces = [tuple(reversed(range(n))), tuple(range(n, 2*n))]
        faces.extend((i, (i+1) % n, (i+1) % n+n, i+n) for i in range(n))
        return call("mesh", name, verts, faces, mat, bevel=bevel)

    def strip(name, a, b, width, y, depth, mat="edge", bevel=.0002):
        ax, az = api["p"](*a)
        bx, bz = api["p"](*b)
        dx, dz = bx-ax, bz-az
        length = math.hypot(dx, dz)
        if length <= 1e-8:
            raise ValueError("Zero-length strip: " + name)
        nx, nz = -dz/length*width/2, dx/length*width/2
        polygon = [(ax+nx, az+nz), (bx+nx, bz+nz),
                   (bx-nx, bz-nz), (ax-nx, az-nz)]
        n = len(polygon)
        verts = [(x, yy, z) for yy in (y-depth/2, y+depth/2)
                 for x, z in polygon]
        faces = [(3, 2, 1, 0), (4, 5, 6, 7)]
        faces += [(i, (i+1) % n, (i+1) % n+n, i+n) for i in range(n)]
        return call("mesh", name, verts, faces, mat, bevel=bevel)

    def window(name, outer, inner, width, mat="polymer", bevel=.0007):
        """An extruded ring with a genuine through opening, no dark cap."""
        if len(outer) != len(inner):
            raise ValueError("Ring contours must have equal vertex counts")
        outer, inner = pts(outer), pts(inner)
        n = len(outer)
        verts = [(x, y, z) for y in (-width/2, width/2)
                 for contour in (outer, inner) for x, z in contour]
        faces = []
        for i in range(n):
            j = (i+1) % n
            faces += [(i, j, n+j, n+i),
                      (2*n+i, 3*n+i, 3*n+j, 2*n+j),
                      (i, 2*n+i, 2*n+j, j),
                      (n+i, n+j, 3*n+j, 3*n+i)]
        return call("mesh", name, verts, faces, mat, bevel=bevel)

    def bolt(name, px, py, y, radius=.0031, mat="metal"):
        side = 1 if y > 0 else -1
        call("cyl", name + "_Rim", xyz(px, py, y), radius*1.21,
             .0010, "edge", axis="Y", vertices=32)
        call("cyl", name + "_Head", xyz(px, py, y+side*.00065), radius,
             .0012, mat, axis="Y", vertices=24)
        call("cyl", name + "_Socket", xyz(px, py, y+side*.00133), radius*.40,
             .0002, "dark", axis="Y", vertices=6)

    def oval_tube(name, stations, width_radii, height_radii,
                  py_centres, mat="polymer", segments=64):
        """Circular/oval, multi-section cheek tube with softly rounded ends."""
        verts = []
        for px, rw, rh, py in zip(stations, width_radii, height_radii, py_centres):
            x, z = api["p"](px, py)
            for i in range(segments):
                t = i*math.tau/segments
                verts.append((x, math.sin(t)*rw, z+math.cos(t)*rh))
        count = len(stations)
        faces = [tuple(reversed(range(segments))),
                 tuple(range((count-1)*segments, count*segments))]
        for row in range(count-1):
            for i in range(segments):
                j = (i+1) % segments
                faces.append((row*segments+i, row*segments+j,
                              (row+1)*segments+j, (row+1)*segments+i))
        obj = call("mesh", name, verts, faces, mat, bevel=.0003)
        if getattr(obj, "data", None) is not None:
            for polygon in obj.data.polygons:
                polygon.use_smooth = len(polygon.vertices) == 4
        return obj

    def rounded_loft(name, silhouette, width, mat="polymer"):
        """Seven Y cross-sections retain the traced, curved grip outline."""
        world = pts(silhouette)
        cx = sum(x for x, z in world)/len(world)
        cz = sum(z for x, z in world)/len(world)
        rings = [(-.50, .934), (-.47, .970), (-.34, .996), (0, 1.0),
                 (.34, .996), (.47, .970), (.50, .934)]
        verts = [(cx+(x-cx)*scale, yy*width, cz+(z-cz)*scale)
                 for yy, scale in rings for x, z in world]
        n = len(world)
        faces = [tuple(reversed(range(n))), tuple(range(6*n, 7*n))]
        for k in range(6):
            for i in range(n):
                j = (i+1) % n
                faces.append((k*n+i, k*n+j, (k+1)*n+j, (k+1)*n+i))
        obj = call("mesh", name, verts, faces, mat, bevel=.0007)
        if getattr(obj, "data", None) is not None:
            for polygon in obj.data.polygons:
                polygon.use_smooth = len(polygon.vertices) == 4
        return obj

    def stipple(name, outline, y, step_px=6, radius=.00033):
        """Grouped tiny moulded relief, clipped to the exact grip inset."""
        def inside(x, z):
            hit = False
            j = len(outline)-1
            for i, (xi, zi) in enumerate(outline):
                xj, zj = outline[j]
                if (zi > z) != (zj > z):
                    if x < (xj-xi)*(z-zi)/(zj-zi) + xi:
                        hit = not hit
                j = i
            return hit
        verts, faces = [], []
        xmin = math.ceil(min(x for x, z in outline)/step_px)*step_px
        xmax = max(x for x, z in outline)
        zmin = math.ceil(min(z for x, z in outline)/step_px)*step_px
        zmax = max(z for x, z in outline)
        sign = 1 if y > 0 else -1
        for py in range(int(zmin), int(zmax), step_px):
            for px in range(int(xmin), int(xmax), step_px):
                jitter_x = ((px*173+py*79) % 11-5)*.17
                jitter_z = ((px*47+py*151) % 13-6)*.13
                xx, zz = px+jitter_x, py+jitter_z
                if not inside(xx, zz):
                    continue
                x, z = api["p"](xx, zz)
                start = len(verts)
                rr = radius*(.83+((px*py) % 17)/55)
                for height, rad in ((0, rr), (.00016*sign, rr*.65)):
                    for k in range(6):
                        angle = k*math.tau/6
                        verts.append((x+math.cos(angle)*rad, y+height,
                                      z+math.sin(angle)*rad))
                faces.append(tuple(range(start+6, start+12)))
                for k in range(6):
                    faces.append((start+k, start+(k+1) % 6,
                                  start+(k+1) % 6+6, start+k+6))
        if verts:
            call("mesh", name, verts, faces, "polymer", bevel=0)

    # --- Standard practical tactical stock: official 2000px default side ---
    begin("SM_M7_Stock_Default", "Stock", "Stock", True)
    prof("FoldingReceiverTongue", [(1608,329), (1632,316), (1683,319),
         (1688,337), (1681,424), (1635,432), (1609,418)], .050, "metal")
    call("cyl", "FoldingVerticalPin", xyz(1657, 369), .0102, .058,
         "metal", axis="Z", vertices=48)
    call("cyl", "UpperHingeCollar", xyz(1657, 316), .0124, .008,
         "edge", axis="Z", vertices=48)
    call("cyl", "UpperHingeCap", xyz(1657, 310), .0094, .004,
         "metal", axis="Z", vertices=48)
    call("cyl", "LowerHingeCollar", xyz(1657, 427), .0122, .005,
         "edge", axis="Z", vertices=48)
    oval_tube("RoundCheekShell", [1668,1674,1687,1718,1908,1936,1947,1951],
              [.021,.026,.028,.028,.028,.028,.024,.017],
              [.017,.024,.024,.024,.024,.024,.023,.018],
              [369,369,369,369,369,369,369,369])
    oval_tube("CheekFrontCuff", [1667,1671,1679,1682],
              [.021,.0278,.0280,.0268], [.017,.0243,.0243,.0235],
              [369]*4, "metal", 48)
    oval_tube("CheekTailCuff", [1937,1942,1949,1954],
              [.0270,.0290,.0260,.0190], [.0238,.0248,.0240,.0190],
              [369]*4, "metal", 48)
    # Real triangular opening, surrounded by the curved structural frame.
    window("DeepTriangularLowerFrame",
           [(1748,478),(1924,480),(1945,441),(1970,384),(1963,493),
            (1954,551),(1938,600),(1915,631),(1884,642),(1863,630),
            (1830,568),(1796,525)],
           [(1789,497),(1907,497),(1928,474),(1944,458),(1938,514),
            (1929,554),(1915,588),(1898,612),(1885,619),(1877,610),
            (1853,563),(1826,520)], .031, "polymer", .0009)
    # Four independent transverse braces leave the cavity open between them.
    for i, contour in enumerate((
        [(1718,479),(1939,480),(1936,489),(1718,488)],
        [(1812,518),(1935,518),(1932,528),(1820,528)],
        [(1840,561),(1926,561),(1921,571),(1846,571)],
        [(1866,602),(1907,602),(1900,612),(1872,612)],
    )):
        prof("HorizontalSkeletonBrace_%02d" % i, contour, .024, "metal", .00055)
    prof("LowerForwardStrut", [(1611,466),(1640,465),(1704,473),(1797,474),
         (1818,480),(1816,494),(1807,500),(1631,501),(1610,494)],
         .043, "polymer", .00065)
    prof("BottomTelescopingRail", [(1632,488),(1785,488),(1834,504),
         (1870,513),(1864,519),(1823,515),(1782,503),(1632,502)],
         .025, "metal", .0004)
    prof("HingeLowerColumn", [(1619,412),(1665,419),(1682,450),
         (1683,477),(1675,492),(1636,492),(1620,478)], .043, "metal", .0006)
    # Fan-shaped, narrowly curved shoulder pad follows the full official tail.
    pad = [(1968,309),(1980,320),(1985,343),(1982,382),(1973,448),
           (1963,518),(1948,577),(1927,616),(1902,637),(1882,642),
           (1878,633),(1894,615),(1915,584),(1931,540),(1944,481),
           (1953,417),(1958,357),(1954,326),(1955,312)]
    prof("CurvedRubberShoulderPad", pad, .056, "rubber", .0011)
    prof("ButtInnerBacking", [(1954,314),(1967,313),(1972,341),(1966,406),
         (1954,489),(1942,553),(1924,595),(1900,625),(1885,635),
         (1874,626),(1893,603),(1913,568),(1928,516),(1940,440),
         (1949,366)], .044, "polymer", .0007)
    # Side details: planar layers, adjustment latch, angled mould seams, hardware.
    for side in (-1, 1):
        tag = "L" if side < 0 else "R"
        strip("LowerSpineLip_"+tag, (1637,488),(1799,490), .00115,
              side*.0220, .0008, "edge")
        strip("CheekUpperSeam_"+tag, (1691,331),(1930,331), .0006,
              side*.0145, .00035, "edge", .00008)
        slab("DiagonalMouldedFrontWeb_"+tag,
             [(1695,424),(1720,424),(1777,475),(1752,475)],
             side*.0204, .0016, "polymer")
        for i in range(7):
            xx = 1703+i*6
            strip("WebDiagonalGroove_%s_%02d" % (tag,i),
                  (xx,428),(xx+30,471), .0007, side*.0215,
                  .00045, "dark", .00008)
        slab("LengthLatchPanel_"+tag,
             [(1635,473),(1808,475),(1815,482),(1811,493),(1640,490)],
             side*.0221, .0012, "polymer", .00055)
        bolt("HingePin_"+tag,1657,333,side*.026,.0050)
        bolt("LengthLock_"+tag,1860,505,side*.0175,.0074)
        call("tube", "SlingLoop_"+tag, xyz(1910,465,side*.024),
             .0062, .0045, .004, "metal", axis="Y", segments=40)
        strip("ButtArcInsetA_"+tag,(1965,341),(1951,485),.0012,
              side*.0284,.0006,"dark")
        strip("ButtArcInsetB_"+tag,(1951,485),(1922,590),.0012,
              side*.0284,.0006,"dark")
        strip("ButtArcInsetC_"+tag,(1922,590),(1891,627),.0012,
              side*.0284,.0006,"dark")
    for i in range(18):
        py = 350+i*13
        px = 1984 - max(0,py-390)*.095 - max(0,py-530)*.16
        x,z = api["p"](px,py)
        call("box", "ButtContactTread_%02d" % i, (x,0,z),
             (.0015,.047,.0016), "rubber", bevel=.0003)

    # --- Modified official skeleton marksman stock, same folding mount ---
    begin("SM_M7_Stock_Skeleton", "Stock", "Stock")
    prof("FoldingMountTongue", [(1607,328),(1652,320),(1685,334),
         (1687,415),(1640,425),(1607,413)], .049,"metal")
    call("cyl","FoldingPin",xyz(1641,369),.009,.056,"metal",axis="Z",vertices=48)
    prof("HingeOpenFront",[(1648,334),(1721,338),(1756,357),(1759,400),
         (1726,419),(1660,417),(1653,398)],.042,"metal")
    window("LongOpenSkeletonSpine",
           [(1692,311),(1734,310),(1962,359),(1995,364),(2003,383),
            (1998,421),(1953,431),(1767,423),(1717,418),(1690,395)],
           [(1720,335),(1740,333),(1944,377),(1967,382),(1969,390),
            (1965,401),(1947,410),(1778,404),(1734,396),(1719,382)],
           .039,"metal",.0007)
    prof("SlopedCheekCover",[(1687,302),(1762,315),(1943,350),(1954,364),
         (1942,371),(1768,337),(1689,320)],.054,"polymer",.00085)
    prof("TelescopicLowerRail",[(1750,404),(1938,408),(1972,402),
         (1976,413),(1937,423),(1744,419)],.025,"metal",.00045)
    prof("TailPivotHousing",[(1974,350),(2015,345),(2042,361),(2043,416),
         (2018,437),(1984,433),(1974,413)],.046,"polymer",.0008)
    skel_outer=[(2016,346),(2046,356),(2055,379),(2047,427),
                (2034,493),(2015,553),(1996,591),(1973,596),
                (1963,576),(1976,539),(1991,488),(2005,423)]
    skel_inner=[(2026,371),(2030,378),(2032,392),(2025,425),
                (2014,475),(1998,524),(1988,550),(1980,555),
                (1981,545),(1991,513),(2006,469),(2017,417)]
    window("AngledButtFrame",skel_outer,skel_inner,.044,"polymer",.00095)
    prof("ThinButtRubber",[(2041,358),(2055,371),(2058,393),(2048,445),
         (2034,507),(2014,563),(2000,588),(1975,598),(1970,588),
         (1989,576),(2007,542),(2023,489),(2036,429),(2043,383)],
         .050,"rubber",.0010)
    for side in (-1,1):
        tag="L" if side<0 else "R"
        for name,px,py,r in (("Mount",1643,350,.0044),
                             ("Adjust",1929,391,.0057),
                             ("TailPivot",2020,391,.0063)):
            bolt(name+"_"+tag,px,py,side*.024,r)
        call("tube","TailSlingEye_"+tag,xyz(2040,423,side*.0255),
             .0077,.0048,.0035,"edge",axis="Y",segments=48)
        strip("BottomRailSeam_"+tag,(1782,415),(1936,419),.001,
              side*.0202,.0007,"edge")
        slab("ButtRaisedCheek_"+tag,[(2017,431),(2025,443),(2014,503),
             (1996,550),(1988,553),(1998,520),(2011,473)],side*.0225,
             .0012,"polymer",.00045)
        for i in range(4):
            slab("LowerRib_%s_%02d"%(tag,i),[(1706+i*8,389),(1712+i*8,385),
                 (1732+i*8,404),(1726+i*8,409)],side*.021,.0012,
                 "polymer",.0003)

    # --- Short straight official 20-round box, then extended box variant ---
    for extended in (False, True):
        begin("SM_M7_Mag_Extended" if extended else "SM_M7_Mag_20",
              "Magazine", "Magazine", not extended)
        extra = 110 if extended else 0
        body=[(999,576),(1200,637),(1201,788+extra),(1207,805+extra),
              (1207,821+extra),(1196,830+extra),(1006,830+extra),
              (995,821+extra),(995,633),(997,597)]
        prof("StraightBoxBody",body,.042,"polymer",.00075)
        prof("UpperSlantedCollar",[(998,578),(1201,636),(1202,656),
             (994,599)],.044,"metal",.0005)
        prof("FloorPlate",[(995,793+extra),(1203,793+extra),
             (1210,807+extra),(1209,823+extra),(1201,831+extra),
             (1004,831+extra),(994,824+extra)],.047,"polymer",.0007)
        prof("FloorPlateBottomLip",[(995,815+extra),(1208,815+extra),
             (1209,826+extra),(1201,835+extra),(1002,835+extra),
             (994,827+extra)],.0478,"rubber",.00045)
        for side in (-1,1):
            tag="L" if side<0 else "R"
            y=side*.02155
            # The front narrow panel is split into three broad horizontal blocks.
            panels = [([(1002,607),(1061,624),(1061,646),(1001,631)],"UpperFront"),
                      ([(1000,643),(1060,657),(1060,695),(1000,680)],"MidFront"),
                      ([(1000,691),(1061,706),(1061,779+extra),
                        (1000,779+extra)],"LowerFront"),
                      ([(1069,629),(1178,659),(1180,785+extra),
                        (1070,785+extra)],"BroadFlatSide")]
            for panel,name in panels:
                slab(name+"_"+tag,panel,y,.0011,"polymer",.00035)
            strip("SlantedUpperSeam_"+tag,(1001,604),(1186,657),.00075,
                  side*.02225,.00055,"dark")
            strip("RearEdgeRib_"+tag,(1190,653),(1195,785+extra),.0022,
                  side*.02215,.00125,"edge",.00025)
            strip("FrontSideDivision_"+tag,(1066,625),(1067,782+extra),.00075,
                  side*.02224,.0005,"dark")
            strip("FirstHorizontalMouldSeam_"+tag,(1000,682),(1065,698),
                  .0012,side*.0223,.0005,"dark")
            strip("SecondHorizontalMouldSeam_"+tag,(1000,709),(1065,719),
                  .0011,side*.0223,.0005,"dark")
            strip("FrontLowerBorder_"+tag,(1002,778+extra),(1058,779+extra),
                  .0011,side*.0224,.0006,"edge")
            slab("FloorPlateInset_"+tag,[(1012,800+extra),(1190,800+extra),
                 (1197,814+extra),(1010,814+extra)],side*.0242,.001,
                 "polymer",.00035)
            if extended:
                strip("ExtensionMouldSeam_"+tag,(1000,790),(1183,790),
                      .001,side*.0223,.00065,"dark")
                slab("ExtendedLowerPanel_"+tag,[(1076,809),(1174,809),
                     (1174,882),(1076,882)],side*.0222,.001,
                     "polymer",.0003)
        # Rear/back column ribs, visible from three-quarter angles.
        for i in range(6 if extended else 4):
            x,z=api["p"](1202,670+i*31)
            call("box","RearThinTransverseRidge_%02d"%i,(x,0,z),
                 (.0012,.042,.00115),"edge",bevel=.00025)
        call("box","BottomLockCatch",xyz(1100,830+extra),
             (.013,.024,.0018),"polymer",bevel=.0005)

    # --- Default curved grip and D-2 ribbed marksman grip ---
    default_outline=[(1340,536),(1355,533),(1400,533),(1420,527),
        (1432,505),(1453,487),(1478,476),(1496,480),(1495,504),
        (1470,514),(1458,526),(1453,541),(1455,559),(1474,581),
        (1491,615),(1510,660),(1531,711),(1546,754),(1549,773),
        (1543,785),(1417,785),(1416,762),(1410,739),(1395,700),
        (1381,659),(1368,627),(1358,602),(1350,580),(1343,561)]
    default_panel=[(1369,619),(1409,622),(1427,625),(1438,641),
        (1452,673),(1472,718),(1480,741),(1473,751),
        (1457,763),(1432,775),(1418,765),(1412,746),
        (1399,710),(1384,671)]
    d2_outline=[(1340,536),(1355,533),(1400,533),(1420,527),
        (1432,505),(1453,487),(1478,476),(1496,480),(1495,504),
        (1470,514),(1458,527),(1453,541),(1459,566),(1478,594),
        (1498,632),(1518,675),(1538,718),(1548,741),(1544,754),
        (1441,794),(1427,786),(1419,770),(1411,746),(1405,742),
        (1407,734),(1401,725),(1395,721),(1397,713),(1391,704),
        (1385,700),(1387,692),(1381,683),(1375,679),(1377,671),
        (1371,662),(1365,658),(1367,650),(1361,641),(1355,637),
        (1357,629),(1351,620),(1347,615),(1349,607),(1343,598),
        (1340,589),(1342,581),(1340,565)]
    for d2 in (False,True):
        begin("SM_M7_Grip_D2" if d2 else "SM_M7_Grip_Default",
              "RearGrip","RearGrip",not d2)
        outline=d2_outline if d2 else default_outline
        rounded_loft("CurvedMouldedShell",outline,.046 if d2 else .048)
        prof("UpperReceiverGripTongue",[(1346,535),(1419,529),(1434,520),
             (1445,536),(1437,559),(1353,561)],.038,"polymer",.0007)
        if d2:
            prof("AngledRubberBase",[(1424,773),(1544,735),(1550,748),
                 (1546,761),(1443,801),(1429,793)],.049,"rubber",.0007)
        else:
            prof("RoundedBottomHeel",[(1417,764),(1548,764),(1547,784),
                 (1539,790),(1425,790),(1417,785)],.047,"polymer",.0008)
            prof("ThinHeelCapLip",[(1417,781),(1545,781),(1544,790),
                 (1425,790),(1417,786)],.049,"rubber",.0004)
        for side in (-1,1):
            tag="L" if side<0 else "R"
            sy=side*(.02355 if d2 else .02455)
            if d2:
                panel=[(1384,601),(1420,597),(1452,630),(1493,711),
                       (1502,737),(1477,753),(1447,764),(1428,733),
                       (1404,679)]
                slab("RaisedAngularSidePanel_"+tag,panel,sy,.0013,
                     "polymer",.0006)
                strip("RearDiagonalPanelSeam_"+tag,(1458,610),(1532,743),
                      .0013,sy+side*.0008,.0007,"edge")
                strip("LowerPanelSeam_"+tag,(1437,776),(1528,744),
                      .0013,sy+side*.0007,.0007,"edge")
                strip("FrontInsetEdge_"+tag,(1384,601),(1428,733),
                      .0012,sy+side*.0007,.0006,"dark")
                # Nine real forward ribs have scalloped edge steps in the loft.
                for i in range(9):
                    py=588+i*18
                    px=1346+i*8
                    slab("FingerGroove_%s_%02d"%(tag,i),
                         [(px,py),(px+34,py+9),(px+37,py+14),(px+2,py+6)],
                         side*.0228,.001,"dark",.00025)
                    if side == 1:
                        prof("FingerCrossRib_%02d"%i,
                             [(px-3,py+3),(px+5,py+5),(px+9,py+12),(px,py+10)],
                             .046,"polymer",.0005)
                stipple("D2FinePanelTexture_"+tag,panel,sy+side*.00075,
                        step_px=7,radius=.00029)
            else:
                slab("InsetDarkGripPanel_"+tag,default_panel,sy,
                     .0010,"rubber",.0005)
                # Border is a genuine narrow relief, not conspicuous decoration.
                for i in range(len(default_panel)):
                    strip("InsetBorder_%s_%02d"%(tag,i),default_panel[i],
                          default_panel[(i+1)%len(default_panel)],.00075,
                          sy+side*.0005,.00035,"polymer",.00010)
                stipple("MouldedPanelStipple_"+tag,default_panel,
                        sy+side*.00055,step_px=5,radius=.00030)
                slab("RearBackstrapRelief_"+tag,
                     [(1480,594),(1496,624),(1527,704),(1542,747),
                      (1532,739),(1512,729),(1497,702),(1469,634),
                      (1458,611)],side*.0246,.00075,"polymer",.00045)
            bolt("UpperGripFixing_"+tag,1387,557,side*.024,.0026)
        # Front and rear checkering runs across the rounded narrow faces.
        for i in range(14 if not d2 else 11):
            py=630+i*9
            px=1368+(py-630)*.35 if not d2 else 1363+(py-630)*.45
            x,z=api["p"](px,py)
            call("box","FrontCrossTexture_%02d"%i,(x,0,z),
                 (.00085,.031,.0007),"polymer",bevel=.00017)
        call("box","BottomRemovablePlug",xyz(1478,781 if not d2 else 783),
             (.019,.025,.0017),"dark",bevel=.00045)

    if tuple(result) != ASSET_NAMES:
        raise AssertionError("Furniture asset ordering changed unexpectedly")
    if any(not record["objects"] for record in result.values()):
        raise AssertionError("Furniture asset is empty")
    return result
