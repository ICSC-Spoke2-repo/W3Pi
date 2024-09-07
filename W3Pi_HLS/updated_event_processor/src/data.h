#ifndef DATA_H
#define DATA_H

#include "ap_int.h"
#include "ap_fixed.h"
#include <cstdint>
#include <fstream>
#include "math.h"

#define NPUPPI_MAX 208                    // Max number of input puppi candidates (208 from Puppi)
#define NSUBARR 8                         // Number of sub-arrays
#define NSPLITS ( NPUPPI_MAX / NSUBARR )  // Dimention of each sub-arrays
#define NPUPPI_SEL 10                     // Max number of selected non-masked ordered candidates
#define NTRIPLETS ( NPUPPI_SEL - 2 )      // Number of triplets: candidates minos 2 pivots

// Index type - should always be able to cover [0,NPUPPI_MAX] !
typedef ap_uint<8> idx_t; // [0,255]

// DeltaR type
typedef ap_uint<24> dr2_t;

// Cosine type
typedef ap_int<10> cos_t; // 1 unit = 1/512

// Hyperbolic-Cosine type
typedef ap_uint<10> cosh_t; // 1 unit = 1/1024 (note the unsigned type)

// Invariant mass types
typedef ap_ufixed<16,10,AP_RND,AP_SAT> mass_t;

// Puppi class
struct Puppi {
    // data types and constants
    typedef ap_ufixed<14,12,AP_RND,AP_SAT> pt_t;
    typedef ap_int<12> eta_t;
    typedef ap_int<11> phi_t;
    typedef ap_int<10> z0_t;
    static constexpr int INT_PI = 360;
    static constexpr int INT_2PI = 2*INT_PI;
    static constexpr float ETAPHI_LSB = M_PI/(2*INT_PI); // pi / 720 = 1/4 deg = 3.14159 / 720 = 0.0043633194
    static constexpr float ETA_CUT = 2.4/ETAPHI_LSB;
    static constexpr float Z0_LSB = 0.5; // mm
    enum PID {H0=0, Gamma=1, HMinus=2, HPlus=3, EMinus=4, EPlus=5, MuMinus=6, MuPlus=7};

    // data members
    pt_t hwPt;
    eta_t hwEta;
    phi_t hwPhi;
    ap_uint<3> hwID;
    z0_t hwZ0;

    // pack and unpack
    uint64_t pack() const {
        ap_uint<64> ret;
        ret(13,0)  = hwPt(13,0);
        ret(25,14) = hwEta(11,0);
        ret(36,26) = hwPhi(10,0);
        ret(39,37) = hwID(2,0);
        ret(49,40) = hwZ0(9,0);
        return ret.to_uint64();
    }
    Puppi & unpack(uint64_t packed) { return unpack(ap_uint<64>(packed)); }
    Puppi & unpack(ap_uint<64> packed) {
       hwPt(13,0)  = packed(13,0);
       hwEta(11,0) = packed(25,14);
       hwPhi(10,0) = packed(36,26);
       hwID(2,0)   = packed(39,37);
       hwZ0(9,0)   = packed(49,40);
       return *this;
    }

    // clear candidate, i.e. fill with zeros
    void clear() {
        hwPt = 0;
        hwEta = 0;
        hwPhi = 0;
        hwID = 0;
        hwZ0 = 0;
    }

    // get charge from ID
    int charge() const {
        if (hwID <= 1) return 0;
        else if ( hwID == 2 || hwID == 4 || hwID == 6) return -1;
        else return 1;
    }

    // covenience functions for printout
    // methods
    float floatPt()  const { return floatPt(hwPt); }
    float floatEta() const { return floatEta(hwEta); }
    float floatPhi() const { return floatPhi(hwPhi); }
    float floatZ0()  const { return floatZ0(hwZ0); }
    // helpers
    static pt_t  toHwPt(float pt)      { return pt_t(pt); }
    static eta_t toHwEta(float eta)    { return eta_t(round(eta/ETAPHI_LSB)); }
    static phi_t toHwPhi(float phi)    { return phi_t(round(phi/ETAPHI_LSB)); }
    static z0_t  toHwZ0(float z0)      { return z0_t(round(z0/Z0_LSB)); }
    static float floatPt(pt_t hwPt)    { return hwPt.to_float(); }
    static float floatEta(eta_t hwEta) { return hwEta.to_int() * ETAPHI_LSB; }
    static float floatPhi(phi_t hwPhi) { return hwPhi.to_int() * ETAPHI_LSB; }
    static float floatZ0(z0_t hwZ0)    { return hwZ0.to_int()  * Z0_LSB; }

    // Overload difference operator
    bool operator != (const Puppi& a) const
    {
        return ( (floatPt(hwPt) != a.floatPt()) || (floatEta(hwEta) != a.floatEta()) || (floatPhi(hwPhi) != a.floatPhi()) );
    }

    // Overload "less" operator
    bool operator < (const Puppi& a) const
    {
        return ( hwPt <= a.hwPt );
    }

    // Overload ostream operator
    friend std::ostream& operator << (std::ostream& os, const Puppi& a)
    {
        //os << a.floatPt() << "/" << a.floatEta() << "/" << a.floatPhi();
        //os << a.floatPt() << ":" << a.floatEta() << "   ";
        os << a.floatPt();
        return os;
    }
};

#endif
