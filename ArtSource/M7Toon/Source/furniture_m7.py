"""M7 inspired visual-prop furniture: slim folding stock, grip and 20 round mag.

The public helper contract is ``build_furniture(model_api)``.  Geometry faces +X,
all dimensions are metres.  The stock's central opening is an actual mesh hole.
These are exterior art shapes, with no internal firearm mechanism.
"""
import math


def build_furniture(api):
    result = {"stock": [], "grip": [], "magazine": []}

    def add(group, fn, *args, **kwargs):
        helper = api[fn] if isinstance(api, dict) else getattr(api, fn)
        obj = helper(*args, **kwargs)
        result[group].append(obj)
        return obj

    def profile(group, name, pts, width, mat, bevel=.001):
        return add(group, "profile", name, pts, width, mat, bevel=bevel)

    def slab(group, name, pts, side, depth, mat, bevel=.0005):
        """A thin relief polygon in an XZ side face."""
        n = len(pts)
        y0, y1 = side-depth/2, side+depth/2
        vs = [(x, y, z) for y in (y0, y1) for x, z in pts]
        fs = [tuple(reversed(range(n))), tuple(range(n, 2*n))]
        fs += [(i, (i+1) % n, (i+1) % n+n, i+n) for i in range(n)]
        return add(group, "mesh", name, vs, fs, mat, bevel=bevel)

    def strip(group, name, a, b, breadth, side, depth, mat, bevel=.0003):
        dx, dz = b[0]-a[0], b[1]-a[1]
        length = math.hypot(dx, dz)
        nx, nz = -dz/length*breadth/2, dx/length*breadth/2
        pts = [(a[0]+nx, a[1]+nz), (b[0]+nx, b[1]+nz),
               (b[0]-nx, b[1]-nz), (a[0]-nx, a[1]-nz)]
        return slab(group, name, pts, side, depth, mat, bevel)

    def ring(group, name, outer, inner, width, mat, bevel=.001):
        """Extrude an annular polygon without capping its through opening."""
        n = len(outer)
        assert n == len(inner)
        vs = [(x, y, z) for y in (-width/2, width/2)
              for contour in (outer, inner) for x, z in contour]
        fs = []
        for i in range(n):
            j = (i+1) % n
            fs.extend([(i, j, n+j, n+i),
                       (2*n+i, 3*n+i, 3*n+j, 2*n+j),
                       (i, 2*n+i, 2*n+j, j),
                       (n+i, n+j, 3*n+j, 3*n+i)])
        return add(group, "mesh", name, vs, fs, mat, bevel=bevel)

    def bolt(group, name, x, y, z, radius=.0035):
        add(group, "cyl", name+"_Washer", (x,y,z), radius+.0012, .0012,
            "edge", axis="Y", vertices=32)
        add(group, "cyl", name+"_Head", (x,y+math.copysign(.0007,y),z),
            radius, .0012, "metal", axis="Y", vertices=12)
        add(group, "cyl", name+"_Socket", (x,y+math.copysign(.0014,y),z),
            radius*.43, .00035, "dark", axis="Y", vertices=6)

    # The real M7 stock is a narrow, front-high wedge rather than an AR buffer
    # tube.  An eight-sided extruded ring creates its long central window.
    outer = [(-.215,.038), (-.398,.014), (-.414,.005), (-.409,-.018),
             (-.242,-.022), (-.220,-.014), (-.207,-.002), (-.207,.025)]
    inner = [(-.248,.020), (-.380,.003), (-.392,-.003), (-.385,-.006),
             (-.250,-.010), (-.240,-.007), (-.236,.001), (-.237,.017)]
    ring("stock", "Stock_OpenSkeletonFrame", outer, inner, .047, "metal", .00125)
    profile("stock", "Stock_LowCheekSpine", [(-.218,.040),(-.376,.019),
        (-.389,.011),(-.374,.008),(-.242,.025),(-.218,.026)], .052,
        "polymer", .0017)
    profile("stock", "Stock_FrontFoldingTongue", [(-.188,.023),(-.208,.027),
        (-.226,.012),(-.227,-.016),(-.188,-.016)], .050, "metal", .0012)
    profile("stock", "Stock_AdjustmentHeel", [(-.386,.014),(-.402,.016),
        (-.420,.007),(-.419,-.025),(-.404,-.029),(-.387,-.010)], .052,
        "polymer", .00135)
    # The butt has a rounded heel and a narrow sloped shoulder-contact edge.
    butt_pts = [(-.420,.027),(-.440,.023),(-.452,.008),(-.451,-.026),
        (-.442,-.067),(-.429,-.098),(-.420,-.105),(-.408,-.104),
        (-.401,-.097),(-.407,-.088),(-.418,-.045),(-.416,-.020),
        (-.407,.000),(-.408,.018)]
    profile("stock", "Stock_ContouredButtShell", butt_pts, .049, "polymer", .002)
    pad_pts = [(-.442,.024),(-.452,.013),(-.455,-.008),(-.451,-.040),
        (-.438,-.080),(-.428,-.101),(-.420,-.105),(-.416,-.101),
        (-.431,-.068),(-.443,-.026),(-.444,.009),(-.436,.018)]
    profile("stock", "Stock_ThinRubberShoulderPad", pad_pts, .054, "rubber", .0015)
    for s in (-1, 1):
        y = s*.0245
        strip("stock", "Stock_SpineSeam_%s" % s, (-.251,.026),(-.370,.010),
              .0011,y,.0007,"dark",.00022)
        strip("stock", "Stock_LowerWindowLip_%s" % s, (-.250,-.012),(-.380,-.008),
              .0014,y,.0008,"edge",.0003)
        slab("stock", "Stock_HeelRecess_%s" % s,
             [(-.418,-.029),(-.428,-.042),(-.421,-.075),(-.411,-.089),
              (-.406,-.086),(-.415,-.054)],s*.025,.001,"dark",.001)
        bolt("stock", "Stock_StockLengthLock_%s" % s,-.395,s*.0271,-.004,.0051)
        bolt("stock", "Stock_ButtPivot_%s" % s,-.428,s*.0267,.005,.0061)
        # Three tiny longitudinal witness steps in the adjustment spine.
        for i in range(3):
            x=-.315-i*.020
            strip("stock", "Stock_AdjustmentMark_%s_%02d" % (s,i),
                  (x,.004),(x-.006,.0032),.00065,s*.0241,.0004,"label",.00015)
    # Small transverse rubber ridges follow the sloped rear contact surface.
    for i in range(12):
        z=.004-i*.0082
        t=max(0,(-z-.01)/.083)
        x=-.451+.028*t
        add("stock","box","Stock_PadRib_%02d" % i,(x-.0004,0,z),
            (.0017,.052,.00165),"rubber",bevel=.0006)
    add("stock","tube","Stock_RearSlingEye",(-.417,0,-.072),.0056,.0033,
        .052,"metal",axis="Y",segments=32)

    # A swept solid combat grip has a realistic rear rake and a broad radiused
    # palm back, with discreet rubber side panels, seams and packed stippling.
    grip_pts = [(-.110,-.059),(-.052,-.064),(-.051,-.079),(-.061,-.092),
        (-.086,-.154),(-.101,-.184),(-.109,-.191),(-.144,-.191),
        (-.151,-.181),(-.132,-.138),(-.112,-.080)]
    profile("grip", "Grip_RakedPolymerCore", grip_pts, .041, "polymer", .0021)
    profile("grip", "Grip_Backstrap", [(-.112,-.077),(-.120,-.094),
        (-.138,-.143),(-.153,-.179),(-.147,-.188),(-.142,-.182),
        (-.124,-.138),(-.105,-.086)], .0405, "rubber", .0013)
    side_pts = [(-.104,-.091),(-.067,-.097),(-.090,-.154),(-.109,-.180),
        (-.136,-.180),(-.126,-.151)]
    for s in (-1,1):
        slab("grip", "Grip_RubberSideInsert_%s" % s,side_pts,s*.021,.0016,
             "rubber",.0011)
        strip("grip","Grip_FrontPanelBorder_%s" % s,(-.066,-.100),(-.101,-.177),
              .0014,s*.022,.0008,"polymer",.0004)
        strip("grip","Grip_RearPanelBorder_%s" % s,(-.107,-.099),(-.135,-.174),
              .00135,s*.022,.0008,"polymer",.0004)
        # All diamonds on one side form one mesh, so stippling is editable
        # without hundreds of independent Blender objects.
        vs, fs = [], []
        for row in range(22):
            z=-.103-row*.0033
            t=(abs(z)-.103)/.073
            rear=-.108-.027*t
            front=-.069-.033*t
            for col in range(9):
                x=rear+.004+(front-rear-.008)*(col+.5*(row%2))/9
                y=s*.02205
                base=len(vs)
                rx, rz=.00052,.0008
                # Pyramidal relief catches broad lighting while keeping
                # a restrained single black rubber material.
                vs += [(x-rx,y,z),(x,y,z-rz),(x+rx,y,z),(x,y,z+rz),
                       (x,y+s*.00045,z)]
                fs += [(base,base+1,base+4),(base+1,base+2,base+4),
                       (base+2,base+3,base+4),(base+3,base,base+4),
                       (base+3,base+2,base+1,base)]
        add("grip","mesh","Grip_DenseRubberStipple_%s" % s,vs,fs,"rubber",bevel=0)
        bolt("grip","Grip_UpperPanelFastener_%s" % s,-.091,s*.0225,-.087,.0021)
    profile("grip","Grip_BottomPlug",[(-.106,-.183),(-.139,-.183),
        (-.144,-.188),(-.139,-.192),(-.108,-.192),(-.103,-.187)],.040,
        "rubber",.001)
    for i in range(12):
        z=-.103-i*.0062
        x=-.111-(abs(z)-.093)*.43
        add("grip","box","Grip_BackstrapGripRib_%02d" % i,(x,0,z),
            (.0015,.035,.0013),"rubber",bevel=.0005)

    # Short and broad 20-round visual silhouette.  The modest swept centreline
    # avoids the long banana shape that reads as a different rifle family.
    levels=[(-.058,.0690,.086,.049),(-.081,.0692,.087,.050),
            (-.112,.0705,.088,.050),(-.145,.0732,.089,.050),
            (-.172,.0770,.090,.050),(-.199,.0810,.091,.050),
            (-.209,.0815,.091,.049)]
    verts=[]
    for z,cx,wx,wy in levels:
        ch=.0028
        corners=[(-wx/2+ch,-wy/2),(wx/2-ch,-wy/2),(wx/2,-wy/2+ch),
                 (wx/2,wy/2-ch),(wx/2-ch,wy/2),(-wx/2+ch,wy/2),
                 (-wx/2,wy/2-ch),(-wx/2,-wy/2+ch)]
        verts += [(cx+x,y,z) for x,y in corners]
    faces=[tuple(reversed(range(8))),tuple(range((len(levels)-1)*8,len(levels)*8))]
    for lev in range(len(levels)-1):
        for k in range(8):
            j=(k+1)%8
            faces.append((lev*8+k,lev*8+j,(lev+1)*8+j,(lev+1)*8+k))
    add("magazine","mesh","Magazine_Short20RoundSweptBody",verts,faces,
        "polymer",bevel=.0012)

    def magcx(z):
        for i in range(len(levels)-1):
            za,ca,wa,ya=levels[i]
            zb,cb,wb,yb=levels[i+1]
            if zb <= z <= za:
                t=(z-za)/(zb-za)
                return ca+(cb-ca)*t
        return levels[-1][1]

    # The side has two broad molded panels, slim vertical edge channels and
    # a few very shallow horizontal seams.  No bright ammunition windows.
    for s in (-1,1):
        y=s*.0255
        for i,(zt,zb) in enumerate([(-.086,-.143),(-.150,-.192)]):
            ct,cb=magcx(zt),magcx(zb)
            pts=[(ct-.030,zt),(ct+.027,zt),(cb+.028,zb),(cb-.030,zb)]
            slab("magazine","Magazine_MoldedSidePanel_%s_%s" % (s,i),
                 pts,y,.0008,"metal",.0007)
            # A low rim and narrow shadow seam create readable molded faces.
            strip("magazine","Magazine_PanelHeader_%s_%s" % (s,i),
                  (ct-.030,zt-.0018),(ct+.026,zt-.0018),.0012,
                  s*.0261,.0006,"polymer",.00025)
        for i,off in enumerate([-.036,-.026,.025,.034]):
            samples=[-.083,-.108,-.142,-.171,-.197]
            for j in range(len(samples)-1):
                za,zb=samples[j:j+2]
                a=(magcx(za)+off,za); b=(magcx(zb)+off,zb)
                strip("magazine","Magazine_FineLongChannel_%s_%s_%s" % (s,i,j),
                      a,b,.00115,s*.0261,.0006,"dark",.00024)
                strip("magazine","Magazine_ChannelLip_%s_%s_%s" % (s,i,j),
                      (a[0]-.0007,za),(b[0]-.0007,zb),.0004,
                      s*.0263,.00035,"edge",.00012)
        for i,z in enumerate([-.080,-.146,-.197]):
            c=magcx(z)
            strip("magazine","Magazine_MoldingCrossSeam_%s_%s" % (s,i),
                  (c-.038,z),(c+.038,z),.00125,s*.0259,.0006,"polymer",.00028)
        # Restrained molded lower shoulder chamfers and the base retention tab.
        slab("magazine","Magazine_BaseMoldedFacet_%s" % s,
             [(.040,-.198),(.055,-.184),(.067,-.183),(.074,-.188),
              (.077,-.202),(.119,-.202),(.126,-.208),(.040,-.209)],
             s*.0258,.0011,"polymer",.0006)
        add("magazine","cyl","Magazine_RetentionDimple_%s" % s,
            (.085,s*.027,-.201),.0021,.0004,"dark",axis="Y",vertices=24)
    profile("magazine","Magazine_ThinFloorplate",[(.032,-.207),(.127,-.207),
        (.131,-.212),(.127,-.216),(.036,-.216),(.032,-.213)],.055,
        "rubber",.0014)
    add("magazine","box","Magazine_FloorplateLockTab",(.080,0,-.215),
        (.018,.025,.003),"polymer",bevel=.0007)
    for i in range(4):
        add("magazine","box","Magazine_BaseAntiSlipRib_%02d" % i,
            (.051+i*.017,0,-.216),(.0014,.044,.0011),"rubber",bevel=.0004)
    return result
