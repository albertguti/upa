

#ifndef UPA_REFERENCEELEMENT_H
#define UPA_REFERENCEELEMENT_H

#include <stdexcept>
#include <cmath>
#include "elementDefinitions.h"

namespace upa {


    /** @brief Reference Element base class, specialised to obtain different types of elements
     */
    class ReferenceElement {

    public:
        ReferenceElement() = default;
        virtual ~ReferenceElement() = default;

        // Getters
        int     getNumGaussPoints() const {return _nG;}
        double* getGaussWeights() {return _gW;}
        double* getGaussCoords() {return _gC;}
        double* getBasisFunctions(int order) {
            if (order == 0) return _bf;
            if (order == 1) return _dbf;
            else return nullptr;
        }
        double* getCurlBF() {return _curlbf;}

        // Evaluate the basis functions within the reference element (using reference coordinates).
        void evaluate(int degree, const double* coords, double *values);
        virtual void evaluateBFs(const double* coords, double *bf) = 0;
        virtual void evaluateDBFs(const double* coords, double *dbf) = 0;

    protected:
        int _dim;
        int _bforder;
        ElemType _elemType;
        BFType _bftype;

        int _nG;      // Number of gauss points
        int _nB;      // Number of basis functions
        double* _gW;  // Gauss weights
        double* _gC;  // Gauss points coordinates
        double* _bf;  // Basis functions evaluated at the gauss points
        double* _dbf; // Derivatives of the basis functions evaluated at the gauss points
        double* _curlbf; // Curl of the basis functions
    };


    /** @brief Templated class, derived from base class.
     *         Template specialisations will create different elements.
     */
    template<ElemType etype, BFType bftype, int>
    class RefElem : public ReferenceElement {
    public:
        RefElem() = default;
        ~RefElem() override = default;

        void evaluateBFs(const double* coords, double *bf) override = 0;
        void evaluateDBFs(const double* coords, double *dbf) override = 0;
    };


    /** @brief Factory method for reference elements.
     *         Takes element parameters and returns corresponding element as a pointer to the base class.
     *
     * @param eType    Element type
     * @param BFType   Basis function type
     * @param BFOrder  Basis function order
     */
    ReferenceElement* getReferenceElement(ElemType eType, BFType BFType, int BFOrder);

}


#endif //FEMPLAS_REFERENCEELEMENT_H
