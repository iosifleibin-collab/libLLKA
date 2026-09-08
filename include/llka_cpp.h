// vim: set sw=4 ts=4 sts=4 expandtab :

/*!
 * @file llka_cpp.h
 * This file provides C++ wrappers around the C API of libLLKA.
 * These bindings are provided primarily to allow for easy binding of JavaScript to libLLKA.
 * Some of the wrappers provided here come with a minor overhead compared to their C counterparts.
 * The C API is the preferred way how to call libLLKA
 */

#ifdef __cplusplus

#ifndef _LLKA_CPP_H
#define _LLKA_CPP_H

#include "llka_main.h"
#include "llka_connectivity_similarity.h"
#include "llka_classification.h"
#include "llka_minicif.h"
#include "llka_ntc.h"
#include "llka_structure.h"

#include <cassert>
#ifndef LLKA_FILESYSTEM_ACCESS_DISABLED
#include <filesystem>
#endif // LLKA_FILESYSTEM_ACCESS_DISABLED
#include <map>
#include <string>
#include <vector>

#define _EMX_GET(type, var)
#define _EMX_SET(type, var)
#define _EMX_GET_SET(type, var)

namespace LLKA {

// Forward declarations (but I still love you, my C++17 sweetie...)
class Atom;
using Structure = std::vector<Atom>;
using Structures = std::vector<Structure>;
using StructureView = std::vector<const Atom *>;
class CifData;

template <typename T, bool>
struct ResultSuccessReturnType;

template <typename S>
struct ResultSuccessReturnType<S, true> {
    using RT = S &;
};

template <typename S>
struct ResultSuccessReturnType<S, false> {
    using RT = S &&;
};

// C++-only generic return type that we return from all function that return a value on success but may fail with an error code
template <typename S, typename F>
class Result {
public:
    template <typename ST = S>
    Result(ST success) :
        m_u{std::move(success)},
        m_isSuccess{true},
        m_movedAway{false}
    {
    }

    Result(F failure, bool) : // The dummy parameter is needed for Embind to be able to differentiate between S and F c-tors
        m_u{std::move(failure)},
        m_isSuccess{false},
        m_movedAway{false}
    {
    }

    Result(const Result &other) :
        m_u{S{}}  // Emscripten needs us to default-init this to something.
    {
        m_u.success.~S(); // Needed because m_u must be default-inited to something

        if (other.m_isSuccess)
            new (&this->m_u.success) S(other.m_u.success);
        else
            new (&this->m_u.failure) F(other.m_u.failure);
        this->m_isSuccess = other.m_isSuccess;
        this->m_movedAway = false;
    }

    Result(Result &&other) noexcept :
        m_u{S{}}  // Emscripten needs us to default-init this to something.

    {
        m_u.success.~S(); // Needed because m_u must be default-inited to something

        if (other.m_isSuccess)
            new (&this->m_u.success) S(std::move(other.m_u.success));
        else
            new (&this->m_u.failure) F(std::move(other.m_u.failure));
        this->m_isSuccess = other.m_isSuccess;
        this->m_movedAway = false;

        other.m_movedAway = true;
    }

    ~Result()
    {
        if (m_movedAway)
            return;
        if (m_isSuccess)
            m_u.success.~S();
        else
            m_u.failure.~F();
    }

    Result & operator=(const Result &other)
    {
        if (this != &other) {
            // Destroy current object
            if (m_isSuccess)
                m_u.success.~S();
            else
                m_u.failure.~F();

            // Construct new object
            if (other.m_isSuccess)
                new (&this->m_u.success) S(other.m_u.success);
            else
                new (&this->m_u.failure) F(other.m_u.failure);
            this->m_isSuccess = other.m_isSuccess;
            this->m_movedAway = false;
        }

        return *this;
    }

    Result & operator=(Result &&other) noexcept
    {
        if (this != &other) {
            // Destroy current object
            if (m_isSuccess)
                m_u.success.~S();
            else
                m_u.failure.~F();

            // Construct new object
            if (other.m_isSuccess)
                new (&this->m_u.success) S(std::move(other.m_u.success));
            else
                new (&this->m_u.failure) F(std::move(other.m_u.failure));
            this->m_isSuccess = other.m_isSuccess;
            this->m_movedAway = false;

            other.m_movedAway = true;
        }

        return *this;
    }

    const F & failure() const
    {
        assert(!m_movedAway);

        if (m_isSuccess)
            throw std::runtime_error{"Cannot get failed value for a succesful result"};
        return m_u.failure;
    }

    typename ResultSuccessReturnType<S, std::is_copy_constructible_v<S>>::RT success()
    {
        assert(!m_movedAway);

        if (!m_isSuccess)
            throw std::runtime_error{"Cannot get success value for a failed result"};

        if constexpr (std::is_copy_constructible_v<S>) {
            return m_u.success;
        } else {
            return std::move(m_u.success);
        }
    }

    template <typename ST = S>
    const ST & success() const
    {
        assert(!m_movedAway);

        if (!m_isSuccess)
            throw std::runtime_error{"Cannot get success value for a failed result"};
        return m_u.success;
    }

    bool isSuccess() const
    {
        assert(!m_movedAway);

        return m_isSuccess;
    }

    template <typename ...Args>
    static Result fail(Args &&...args)
    {
        return Result{F{std::forward<Args>(args)...}, false};
    }

    template <typename ...Args>
    static Result succeed(Args &&...args)
    {
        return Result{S{std::forward<Args>(args)...}};
    }

private:
    union U {
        F failure;
        S success;
        U(S s) noexcept : success{std::move(s)} {}
        template <typename FT = std::enable_if_t<!std::is_same_v<S, F>, F>>
        U(FT f) noexcept : failure{std::move(f)} {}
        ~U() {}
    } m_u;

    bool m_isSuccess;
    bool m_movedAway;
};

template <typename F>
class Result<void, F> {
public:
    Result() noexcept :
        m_isSuccess{true},
        m_movedAway{false}
    {
    }

    Result(F f, bool) noexcept :
        m_failure{std::move(f)},
        m_isSuccess{false},
        m_movedAway{false}
    {
    }

    Result(const Result &other)
    {
        if (!other.m_isSuccess)
            this->m_failure = other.m_failure;

        this->m_isSuccess = other.m_isSuccess;
        this->m_movedAway = false;
    }

    Result(Result &&other) noexcept
    {
        if (!other.m_isSuccess)
            this->m_failure = std::move(other.m_failure);

        this->m_isSuccess = other.m_isSuccess;
        this->m_movedAway = false;

        other.m_movedAway = true;
    }

    Result & operator=(const Result &other)
    {
        if (!other.m_isSuccess)
            this->m_failure = other.m_failure;

        this->m_isSuccess = other.m_isSuccess;
        this->m_movedAway = false;

        return *this;
    }

    Result & operator=(Result &&other) noexcept
    {
        if (!other.m_isSuccess)
            this->m_failure = std::move(m_failure);

        this->m_isSuccess = other.m_isSuccess;
        this->m_movedAway = false;

        other.m_movedAway = true;

        return *this;
    }

    const F & failure() const
    {
        assert(!m_movedAway);

        if (m_isSuccess)
            throw std::runtime_error{"Cannot get failed value for a succesful result"};
        return m_failure;
    }

    bool isSuccess() const
    {
        assert(!m_movedAway);

        return m_isSuccess;
    }

    template <typename ...Args>
    static Result fail(Args &&...args)
    {
        return Result{F{std::forward<Args>(args)...}, false};
    }

    static Result succeed()
    {
        return Result{};
    }

private:
    F m_failure;

    bool m_isSuccess;
    bool m_movedAway;
};

template <typename S>
class Result<S, void> {
public:
    Result(S s) noexcept :
        m_success{std::move(s)},
        m_isSuccess{true},
        m_movedAway{false}
    {
    }

    Result() noexcept :
        m_isSuccess{false},
        m_movedAway{false}
    {
    }

    Result(const Result &other)
    {
        if (other.m_isSuccess)
            this->m_success = other.m_success;

        this->m_isSuccess = other.m_isSuccess;
        this->m_movedAway = false;
    }

    Result(Result &&other) noexcept
    {
        if (other.m_isSuccess)
            this->m_success = std::move(other.m_success);

        this->m_isSuccess = other.m_isSuccess;
        this->m_movedAway = false;

        other.m_movedAway = true;
    }

    Result & operator=(const Result &other)
    {
        if (other.m_isSuccess)
            this->m_success = other.m_success;

        this->m_isSuccess = other.m_isSuccess;
        this->m_movedAway = false;

        return *this;
    }

    Result & operator=(Result &&other) noexcept
    {
        if (other.m_isSuccess)
            this->m_success = std::move(m_success);

        this->m_isSuccess = other.m_isSuccess;
        this->m_movedAway = false;

        other.m_movedAway = true;

        return *this;
    }

    const S & success() const
    {
        assert(!m_movedAway);

        if (!m_isSuccess)
            throw std::runtime_error{"Cannot get success value for a failed result"};
        return m_success;
    }

    bool isSuccess() const
    {
        assert(!m_movedAway);

        return m_isSuccess;
    }

    static Result fail()
    {
        return Result{};
    }

    template <typename ...Args>
    static Result succeed(Args &&...args)
    {
        return Result{S{std::forward<Args>(args)...}};
    }

private:
    S m_success;

    bool m_isSuccess;
    bool m_movedAway;
};

template <typename S> using RCResult = Result<S, LLKA_RetCode>;

//
// Main
//

using Points = std::vector<LLKA_Point>; // TODO: Revise this when we figure out how to do memory alignment of point arrays

class LLKA_CPP_API Matrix {
public:
    explicit Matrix(LLKA_Matrix matrix) noexcept :
        nCols{matrix.nCols},
        nRows{matrix.nRows},
        data{matrix.data}
    {
    }

    explicit Matrix(double *data, size_t nRows, size_t nCols) noexcept :
        nCols{nCols},
        nRows{nRows},
        data{data}
    {
    }

    // Impl is in llka_cpp.cpp
    Matrix(const Matrix &other) noexcept;

    Matrix(Matrix &&other) noexcept :
        nCols{other.nCols},
        nRows{other.nRows},
        data{other.data}
    {
        other.data = nullptr;
    }

    // Impl is in llka_cpp.cpp
    ~Matrix() noexcept;

    const size_t nCols;
    const size_t nRows;

    operator LLKA_Matrix() noexcept
    {
        LLKA_Matrix m{ data, nRows, nCols };
        return m;
    }

    operator const LLKA_Matrix() const noexcept
    {
        LLKA_Matrix m{ data, nRows, nCols };
        return m;
    }

    auto operator()(size_t row, size_t col) noexcept -> double
    {
        assert(row < nRows && col < nCols && "Attempted to get a matrix element that is outside the matrix dimensions");

        return data[nRows * col + row];
    }

    // Impl is in llka_cpp.cpp
    auto operator=(const Matrix &other) noexcept -> Matrix;

    auto operator=(Matrix &&other) noexcept -> Matrix
    {
        const_cast<size_t&>(this->nCols) = other.nCols;
        const_cast<size_t&>(this->nRows) = other.nRows;
        this->data = other.data;

        other.data = nullptr;

        return *this;
    }

private:
    double *data;

};

LLKA_CPP_API
auto operator<<(std::ostream &os, const LLKA_Point &pt) -> std::ostream &;

LLKA_CPP_API
auto errorToString(LLKA_RetCode tRet) -> std::string;

//
// Structure
//

// We cannot take advantage of any "modern" features like constructors or class methods
// because we would have to export this object as a class instead of value_object (== POD)
// in Emscripten builds. Emscripten seems to have major memory management issues when
// complex objects are exported as classes and these issues disappear when we replace them
// with dumb value_objects. #software_is_in_decline
class LLKA_CPP_API Atom {
public:
    std::string type_symbol;
    std::string label_atom_id;
    std::string label_entity_id;
    std::string label_comp_id;
    std::string label_asym_id;
    std::string auth_atom_id;
    std::string auth_comp_id;
    std::string auth_asym_id;
    std::string pdbx_PDB_ins_code;
    LLKA_Point coords;
    uint32_t id;
    int32_t label_seq_id;
    int32_t auth_seq_id;
    int32_t pdbx_PDB_model_num;
    char label_alt_id;
};

LLKA_CPP_API
auto makeEmptyAtom() -> Atom;

LLKA_CPP_API
auto makeAtom(
    std::string type_symbol,
    std::string label_atom_id,
    std::string label_entity_id,
    std::string label_comp_id,
    std::string label_asym_id,
    std::string auth_atom_id,
    std::string auth_comp_id,
    std::string auth_asym_id,
    LLKA_Point coords,
    uint32_t id,
    int32_t label_seq_id,
    int32_t auth_seq_id,
    int32_t pdbx_PDB_model_num,
    std::string pdbx_PDB_ins_code,
    char label_alt_id
) -> Atom;

LLKA_CPP_API
auto atomsEqual(const Atom &a, const Atom &b) noexcept -> bool;

class LLKA_CPP_API AltIdSplit {
public:
    AltIdSplit() = default;

    Structure structure;
    char altId;
};

LLKA_CPP_API
auto operator<<(std::ostream &os, const Atom &atom) -> std::ostream &;

LLKA_CPP_API
auto operator<<(std::ostream &os, const Structure &stru) -> std::ostream &;

inline
auto atomMatches(
    const Atom &atom,
    const std::string &label_atom_id,
    const std::string &label_comp_id,
    const std::string &label_asym_id,
    int32_t label_seq_id,
    char label_alt_id = LLKA_NO_ALTID,
    const std::string &pdbx_PDB_ins_code = LLKA_NO_INSCODE,
    int32_t pdbx_PDB_model_num = 1
) {
    // This function must be kept in sync with atomMatchesCriteria() from structure.cpp
    // Conversion to C API objects and a function call would be too expensive to just
    // call that function.
    if (!label_atom_id.empty() && label_atom_id != atom.label_atom_id)
        return false;

    if (!label_comp_id.empty() && label_comp_id != atom.label_comp_id)
        return false;

    if (!label_asym_id.empty() && label_asym_id != atom.label_asym_id)
        return false;

    if (label_seq_id >= 0 && atom.label_seq_id != label_seq_id)
        return false;

    if (label_alt_id != LLKA_NO_ALTID && atom.label_alt_id != label_alt_id)
        return false;

    if (pdbx_PDB_ins_code != LLKA_NO_INSCODE && pdbx_PDB_ins_code != atom.pdbx_PDB_ins_code)
        return false;

    return atom.pdbx_PDB_model_num == pdbx_PDB_model_num;
}

LLKA_CPP_API
auto compareAtoms(const Atom &a, const Atom &b, bool ignoreId) noexcept -> bool;

LLKA_CPP_API
auto makeStructure(const LLKA_Atom *atoms, size_t nAtoms) -> Structure;

LLKA_CPP_API
auto splitByAltIds(const Structure &stru) -> std::vector<AltIdSplit>;

LLKA_CPP_API
auto splitStructureToDinucleotideSteps(const Structure &stru) -> RCResult<Structures>;

//
// Measurements
//

template <typename T> requires std::is_floating_point_v<T>
LLKA_CPP_API
auto measureAngle(const Atom &a, const Atom &b, const Atom &c) -> T;

template <typename T> requires std::is_floating_point_v<T>
LLKA_CPP_API
auto measureDihedral(const Atom &a, const Atom &b, const Atom &c, const Atom &d) -> T;

template <typename T> requires std::is_floating_point_v<T>
LLKA_CPP_API
auto measureDistance(const Atom &a, const Atom &b) -> T;

//
// Superposition
//

LLKA_CPP_API
auto applyTransformation(Points &what, const Matrix &matrix) noexcept -> RCResult<void>;

LLKA_CPP_API
auto applyTransformation(Structure &what, const Matrix &matrix) noexcept -> RCResult<void>;

LLKA_CPP_API
auto centroid(const Points &points) noexcept -> LLKA_Point;

LLKA_CPP_API
auto centroid(const Structure &stru) noexcept -> LLKA_Point;

LLKA_CPP_API
auto rmsd(const Points &a, const Points &b) noexcept -> RCResult<double>;

LLKA_CPP_API
auto rmsd(const Structure &a, const Structure &b) noexcept -> RCResult<double>;

LLKA_CPP_API
auto superpose(Points &what, const Points &onto) noexcept -> RCResult<double>;

LLKA_CPP_API
auto superpose(Structure &what, const Structure &onto) noexcept -> RCResult<double>;

LLKA_CPP_API
auto superpositionMatrix(Structure &what, const Structure &onto) noexcept -> RCResult<Matrix>;

LLKA_CPP_API
auto superpositionMatrix(Points &what, const Points &onto) noexcept -> RCResult<Matrix>;

LLKA_CPP_API
auto superpositionMatrix(StructureView &what, const StructureView &onto) noexcept -> RCResult<Matrix>;

//
// Segmentation
//

class LLKA_CPP_API StructureSegments {
/*
 * To counter memory management issues that cause memory leaks with Emscripten,
 * we need to export LLKA::Atom class to JavaScript as value_object instead of a class.
 * This, apparently, has the unfortunate consequence of not being able to expose
 * a _pointer_ to LLKA::Atom in the JavaScript bindings. This breaks the originally
 * intended layout of Segmentation mapping.
 * To make Segmentation available in the JavaScript bindings, we need to provide
 * a simplified version that stores copies of atoms instead of pointers.
 * It is dumb but at the moment there does not appear to be a better solution.
 * This is why you will have to deal with the barrage of ifdefs.
 */
public:
#ifdef LLKA_PLATFORM_EMSCRIPTEN
    using Atoms = std::vector<Atom>;
#else
    using Atoms = std::vector<Atom *>;
#endif // LLKA_PLATFORM_EMSCRIPTEN

    struct Residue {
        Atoms atoms;
    };
    using Residues = std::map<int32_t, Residue>;

    struct Chain {
        Residues residues;
#ifndef LLKA_PLATFORM_EMSCRIPTEN
        Atoms atoms;
#endif // LLKA_PLATFORM_EMSCRIPTEN
    };
    using Chains = std::map<std::string, Chain>;

    struct Model {
        Chains chains;
#ifndef LLKA_PLATFORM_EMSCRIPTEN
        Atoms atoms;
#endif // LLKA_PLATFORM_EMSCRIPTEN
    };
    using Models = std::map<int32_t, Model>;

    // Default c-tor makes absolutely no sense but Emscripten requires it
#ifdef LLKA_PLATFORM_EMSCRIPTEN
    StructureSegments() noexcept;
#endif // LLKA_PLATFORM_EMSCRIPTEN
    StructureSegments(Structure &structure) noexcept;

    Models models;
    // We cannot use reference because Emscripten doesn't take it
    Structure *structure;

    _EMX_GET_SET(Models, models)

#ifdef LLKA_PLATFORM_EMSCRIPTEN
    auto _emsGet_structure() const -> const Structure &;
#endif // LLKA_PLATFORM_EMSCRIPTEN
};

//
// NtC
//

class LLKA_CPP_API AtomNameQuad {
public:
    AtomNameQuad()
    {
        a.reserve(sizeof(LLKA_AtomNameQuad::a));
        b.reserve(sizeof(LLKA_AtomNameQuad::b));
        c.reserve(sizeof(LLKA_AtomNameQuad::c));
        d.reserve(sizeof(LLKA_AtomNameQuad::d));
    }

    AtomNameQuad(const AtomNameQuad &) = default;

    AtomNameQuad(AtomNameQuad &&other) noexcept :
        a{std::move(other.a)},
        b{std::move(other.b)},
        c{std::move(other.c)},
        d{std::move(other.d)}
    {}

    AtomNameQuad(std::string _a, std::string _b, std::string _c, std::string _d) noexcept :
        a{std::move(_a)}, b{std::move(_b)}, c{std::move(_c)}, d{std::move(_d)}
    {}

    AtomNameQuad & operator=(const AtomNameQuad &) = default;
    AtomNameQuad & operator=(AtomNameQuad &&other) noexcept
    {
        this->a = std::move(other.a);
        this->b = std::move(other.b);
        this->c = std::move(other.c);
        this->d = std::move(other.d);

        return *this;
    }

    std::string a;
    std::string b;
    std::string c;
    std::string d;

    _EMX_GET_SET(std::string, a)
    _EMX_GET_SET(std::string, b)
    _EMX_GET_SET(std::string, c)
    _EMX_GET_SET(std::string, d)
};

class LLKA_CPP_API BackboneAtom {
public:
    BackboneAtom() = default;

    LLKA_ResidueInStep residue;
    std::string name;
};

LLKA_CPP_API
auto backboneAtomIndex(const BackboneAtom &bkbnAtom, const Structure &backbone) noexcept -> size_t;

LLKA_CPP_API
auto calculateStepMetrics(const Structure &stru) noexcept -> RCResult<LLKA_StepMetrics>;

LLKA_CPP_API
auto calculateStepMetricsDifferenceAgainstReference(const Structure &stru, LLKA_NtC ntc) noexcept -> RCResult<LLKA_StepMetrics>;

LLKA_CPP_API
auto crossResidueMetric(LLKA_CrossResidueMetric metric, const Structure &stru) noexcept -> RCResult<Structure>;

LLKA_CPP_API
auto crossResidueMetricAtoms(const std::string &firstBase, const std::string &secondBase, LLKA_CrossResidueMetric metric) -> RCResult<AtomNameQuad>;

LLKA_CPP_API
auto crossResidueMetricAtoms(const Structure &stru, LLKA_CrossResidueMetric metric) -> RCResult<AtomNameQuad>;

LLKA_CPP_API
auto crossResidueMetricName(LLKA_CrossResidueMetric metric, bool greek) noexcept -> std::string;

LLKA_CPP_API
auto dinucleotideTorsion(LLKA_DinucleotideTorsion torsion, const Structure &stru) -> RCResult<Structure>;

LLKA_CPP_API
auto dinucleotideTorsionAtoms(const std::string &firstBase, const std::string &secondBase, LLKA_DinucleotideTorsion torsion) -> RCResult<AtomNameQuad>;

LLKA_CPP_API
auto dinucleotideTorsionAtoms(const Structure &stru, LLKA_DinucleotideTorsion torsion) -> RCResult<AtomNameQuad>;

LLKA_CPP_API
auto dinucleotideTorsionName(LLKA_DinucleotideTorsion torsion, bool greek) noexcept -> std::string;

LLKA_CPP_API
auto extractBackbone(const Structure &stru) -> RCResult<Structure>;

LLKA_CPP_API
auto extractExtendedBackbone(const Structure &stru) noexcept -> RCResult<Structure>;

LLKA_CPP_API
auto extractMetricsStructure(const Structure &stru) noexcept -> RCResult<Structure>;

LLKA_CPP_API
auto nameToCANA(const std::string &name) noexcept -> LLKA_CANA;

LLKA_CPP_API
auto CANAToName(LLKA_CANA cana) noexcept -> std::string;

LLKA_CPP_API
auto nameToNtC(const std::string &name) noexcept -> LLKA_NtC;

LLKA_CPP_API
auto NtCToName(LLKA_NtC ntc) noexcept -> std::string;

LLKA_CPP_API
auto NtCStructure(LLKA_NtC ntc) noexcept -> Structure;

using StepInfo = LLKA_StepInfo;
LLKA_CPP_API
auto structureIsStep(const Structure &stru) noexcept -> RCResult<LLKA_StepInfo>;

using StepMetrics = LLKA_StepMetrics;
auto operator<<(std::ostream &os, const StepMetrics &metrics) -> std::ostream &;

//
// Connectivity, similarity
//

using Connectivity = LLKA_Connectivity;
using Connectivities = std::vector<Connectivity>;
using Similarity = LLKA_Similarity;
using Similarities = std::vector<LLKA_Similarity>;

LLKA_CPP_API
auto measureStepConnectivity(const Structure &positionFirst, LLKA_NtC ntcFirst, const Structure &positionSecond, LLKA_NtC ntcSecond) noexcept -> RCResult<Connectivity>;

LLKA_CPP_API
auto measureStepConnectivity(const Structure &positionFirst, std::vector<LLKA_NtC> ntcsFirst, const Structure &positionSecond, LLKA_NtC ntcSecond) noexcept -> RCResult<Connectivities>;

LLKA_CPP_API
auto measureStepConnectivity(const Structure &positionFirst, LLKA_NtC ntcFirst, const Structure &positionSecond, std::vector<LLKA_NtC> ntcsSecond) noexcept -> RCResult<Connectivities>;

LLKA_CPP_API
auto measureStepConnectivity(const Structure &positionFirst, const Structure &dinuFirst, const Structure &positionSecond, const Structure &dinuSecond) noexcept -> RCResult<Connectivity>;

LLKA_CPP_API
auto measureStepConnectivity(const Structure &positionFirst, const Structure &dinuFirst, const Structure &positionSecond, const Structures &dinusSecond) noexcept -> RCResult<Connectivities>;

LLKA_CPP_API
auto measureStepSimilarity(const Structure &stepStru, LLKA_NtC ntc) noexcept -> RCResult<Similarity>;

LLKA_CPP_API
auto measureStepSimilarity(const Structure &stepStru, std::vector<LLKA_NtC> ntcs) noexcept -> RCResult<Similarities>;

LLKA_CPP_API
auto measureStepSimilarity(const Structure &stepStru, const Structure &refStru) noexcept -> RCResult<Similarity>;

//
// MiniCif
//

class CifData {
public:
    class Value {
    public:
        std::string text;
        LLKA_CifDataValueState state;
    };
    using Values = std::vector<Value>;

    class Item {
    public:
        std::string keyword;
        Values values;
    };
    using Items = std::vector<Item>;

    class Category {
    public:
        Category() = default;
        Category(std::string name, Items items);

        auto isLoop() -> bool;

        std::string name;
        Items items;

        _EMX_GET_SET(std::string, name)
        _EMX_GET_SET(Items, items)

    };
    using Categories = std::vector<Category>;

    class Block {
    public:
        std::string name;
        Categories categories;
    };
    using Blocks = std::vector<Block>;

    Blocks blocks;
};

class LLKA_CPP_API ImportedStructure {
public:
    ImportedStructure() = default;

    std::string id;
    Structure structure;
    CifData cifData;
};

class LLKA_CPP_API CifError {
public:
    CifError() = default;

    LLKA_RetCode tRet;
    std::string error;
};
using CifResult = Result<ImportedStructure, CifError>;

#ifndef LLKA_FILESYSTEM_ACCESS_DISABLED

LLKA_CPP_API
auto cifToStructure(const std::filesystem::path &path, int32_t options = 0) -> CifResult;

#endif // LLKA_FILESYSTEM_ACCESS_DISABLED

LLKA_CPP_API
auto cifToStructure(const std::string &text, int32_t options = 0) -> CifResult;

LLKA_CPP_API
auto cifDataToString(const CifData &cifData, bool pretty) -> RCResult<std::string>;

//
// Nucleotide
//

LLKA_CPP_API
auto extractNucleotide(const Structure &stru, int32_t pdbx_PDB_model_num, const std::string &label_asym_id, int32_t label_seq_id) -> Structure;

LLKA_CPP_API
auto extractNucleotideView(const Structure &stru, int32_t pdbx_PDB_model_num, const std::string &label_asym_id, int32_t label_seq_id) -> StructureView;

LLKA_CPP_API
auto extractRibose(const Structure &stru) -> RCResult<Structure>;

LLKA_CPP_API
auto extractRiboseView(const Structure &stru) -> RCResult<StructureView>;

LLKA_CPP_API
auto isNucleotideCompound(const std::string &compId) -> bool;

LLKA_CPP_API
auto nameToSugarPucker(const std::string &name) -> LLKA_SugarPucker;

LLKA_CPP_API
auto riboseMetrics(const Structure &stru) -> RCResult<LLKA_RiboseMetrics>;

LLKA_CPP_API
auto riboseMetrics(const StructureView &view) -> RCResult<LLKA_RiboseMetrics>;

LLKA_CPP_API
auto sugarPucker(const Structure &stru) -> RCResult<LLKA_SugarPucker>;

LLKA_CPP_API
auto sugarPucker(const StructureView &stru) -> RCResult<LLKA_SugarPucker>;

LLKA_CPP_API
auto sugarPuckerToName(LLKA_SugarPucker pucker, LLKA_SugarPuckerNameBrevity brevity) -> std::string;

// Classification - declaration of value objects that cannot be mapped
// directly to the C structs

class LLKA_CPP_API GoldenStep {
public:
    GoldenStep() = default;
    GoldenStep(const LLKA_GoldenStep &gs);

    LLKA_SugarPucker pucker_1;
    LLKA_SugarPucker pucker_2;
    LLKA_NuAngles nuAngles_1;
    LLKA_NuAngles nuAngles_2;
    LLKA_StepMetrics metrics;
    std::string name;
    size_t clusterIdx;
    int32_t clusterNumber;
};

//
// Resource loaders
//

#ifndef LLKA_FILESYSTEM_ACCESS_DISABLED

LLKA_CPP_API
auto loadClusterNuAngles(const std::filesystem::path &path) -> RCResult<std::vector<LLKA_ClusterNuAngles>>;

LLKA_CPP_API
auto loadClusters(const std::filesystem::path &path) -> RCResult<std::vector<LLKA_ClassificationCluster>>;

LLKA_CPP_API
auto loadConfals(const std::filesystem::path &path) -> RCResult<std::vector<LLKA_Confal>>;

LLKA_CPP_API
auto loadConfalPercentiles(const std::filesystem::path &path) -> RCResult<std::vector<LLKA_ConfalPercentile>>;

LLKA_CPP_API
auto loadGoldenSteps(const std::filesystem::path &path) -> RCResult<std::vector<GoldenStep>>;

#endif // LLKA_FILESYSTEM_ACCESS_DISABLED

LLKA_CPP_API
auto loadClusterNuAngles(const std::string &text) -> RCResult<std::vector<LLKA_ClusterNuAngles>>;

LLKA_CPP_API
auto loadClusters(const std::string &text) -> RCResult<std::vector<LLKA_ClassificationCluster>>;

LLKA_CPP_API
auto loadConfals(const std::string &text) -> RCResult<std::vector<LLKA_Confal>>;

LLKA_CPP_API
auto loadConfalPercentiles(const std::string &path) -> RCResult<std::vector<LLKA_ConfalPercentile>>;

LLKA_CPP_API
auto loadGoldenSteps(const std::string &text) -> RCResult<std::vector<GoldenStep>>;

//
// Tracing
//

class TracepointInfo {
public:
    int32_t TPID;
    std::string description;
};

LLKA_CPP_API
auto toggleAllTracepoints(bool enable) -> void;

LLKA_CPP_API
auto toggleTracepoint(int32_t TPID, bool enable) -> void;

LLKA_CPP_API
auto trace(bool dontClear = false) -> std::string;

LLKA_CPP_API
auto tracepointInfo() -> std::vector<TracepointInfo>;

LLKA_CPP_API
auto tracepointState(int32_t TPID) -> bool;

//
// Classification
//

class LLKA_CPP_API ClassificationContext {
public:
    ClassificationContext();
    ClassificationContext(LLKA_ClassificationContext *ctx);

    ClassificationContext(const ClassificationContext &other) = delete;

    ClassificationContext(ClassificationContext &&other) noexcept;

    ~ClassificationContext();

    ClassificationContext & operator=(const ClassificationContext &) = delete;
    ClassificationContext & operator=(ClassificationContext &&other) noexcept;

    const LLKA_ClassificationContext * get() const;

    auto isValid() const -> bool;

private:
    LLKA_ClassificationContext *m_ctx;

    bool m_movedAway;
};

class LLKA_CPP_API ClassifiedStep {
public:
    LLKA_NtC assignedNtC;
    LLKA_CANA assignedCANA;
    LLKA_NtC closestNtC;
    LLKA_CANA closestCANA;
    LLKA_ConfalScore confalScore;
    double euclideanDistanceNtCIdeal;
    LLKA_StepMetrics metrics;
    LLKA_StepMetrics differencesFromNtCAverages;
    LLKA_NuAngles nuAngles_1;
    LLKA_NuAngles nuAngles_2;
    double ribosePseudorotation_1;
    double ribosePseudorotation_2;
    double tau_1;
    double tau_2;
    LLKA_SugarPucker sugarPucker_1;
    LLKA_SugarPucker sugarPucker_2;
    LLKA_NuAngles nuAngleDifferences_1;
    LLKA_NuAngles nuAngleDifferences_2;
    double rmsdToClosestNtC;

    std::string closestGoldenStep;

    int32_t violations;

    int16_t violatingTorsionsAverage;
    int16_t violatingTorsionsNearest;

    ClassifiedStep();
    ClassifiedStep(const LLKA_ClassifiedStep &cStep);

    auto hasViolations() const -> bool;
    auto namedViolations() const -> std::vector<std::string>;

    _EMX_GET_SET(LLKA_NtC, assignedNtC)
    _EMX_GET_SET(LLKA_CANA, assignedCANA)
    _EMX_GET_SET(LLKA_NtC, closestNtC)
    _EMX_GET_SET(LLKA_CANA, closestCANA)
    _EMX_GET_SET(LLKA_ConfalScore, confalScore)
    _EMX_GET_SET(double, euclideanDistanceNtCIdeal)
    _EMX_GET_SET(LLKA_StepMetrics, metrics)
    _EMX_GET_SET(LLKA_StepMetrics, differencesFromNtCAverages)
    _EMX_GET_SET(LLKA_NuAngles, nuAngles_1)
    _EMX_GET_SET(LLKA_NuAngles, nuAngles_2)
    _EMX_GET_SET(double, ribosePseudorotation_1)
    _EMX_GET_SET(double, ribosePseudorotation_2)
    _EMX_GET_SET(double, tau_1)
    _EMX_GET_SET(double, tau_2)
    _EMX_GET_SET(LLKA_SugarPucker, sugarPucker_1)
    _EMX_GET_SET(LLKA_SugarPucker, sugarPucker_2)
    _EMX_GET_SET(LLKA_NuAngles, nuAngleDifferences_1)
    _EMX_GET_SET(LLKA_NuAngles, nuAngleDifferences_2)
    _EMX_GET_SET(double, rmsdToClosestNtC)
    _EMX_GET_SET(std::string, closestGoldenStep)
    _EMX_GET_SET(int32_t, violations)
    _EMX_GET_SET(int16_t, violatingTorsionsAverage)
    _EMX_GET_SET(int16_t, violatingTorsionsNearest)
};

class LLKA_CPP_API AttemptedClassifiedStep {
public:
    AttemptedClassifiedStep() = default;

    LLKA_RetCode status;
    ClassifiedStep step;
};
using AttemptedClassifiedSteps = std::vector<AttemptedClassifiedStep>;

LLKA_CPP_API
auto averageConfal(const std::vector<ClassifiedStep> &steps, const ClassificationContext &ctx) noexcept -> LLKA_AverageConfal;

LLKA_CPP_API
auto averageConfal(const std::vector<AttemptedClassifiedStep> &steps, const ClassificationContext &ctx) noexcept -> LLKA_AverageConfal;

LLKA_CPP_API
auto classificationClusterForNtC(LLKA_NtC ntc, const ClassificationContext &ctx) noexcept -> RCResult<LLKA_ClassificationCluster>;

LLKA_CPP_API
auto confalForNtC(LLKA_NtC ntc, const ClassificationContext &ctx) noexcept -> RCResult<LLKA_Confal>;

LLKA_CPP_API
auto confalPercentile(double confalScore, const ClassificationContext &ctx) noexcept -> double;

LLKA_CPP_API
auto classifyStep(const Structure &stru, const ClassificationContext &ctx) -> RCResult<ClassifiedStep>;

LLKA_CPP_API
auto classifySteps(const Structures &strus, const ClassificationContext &ctx) -> RCResult<AttemptedClassifiedSteps>;

LLKA_CPP_API
auto initializeClassificationContext(
    const std::vector<LLKA_ClassificationCluster> &clusters,
    const std::vector<GoldenStep> &goldenSteps,
    const std::vector<LLKA_Confal> &confals,
    const std::vector<LLKA_ClusterNuAngles> &clusterNuAngles,
    const std::vector<LLKA_ConfalPercentile> &confalPercentiles,
    const LLKA_ClassificationLimits &limits,
    double maxCloseEnoughRmsd
) -> RCResult<ClassificationContext>;

} // namespace LLKA

#endif // _LLKA_CPP_H

#endif // __cplusplus
