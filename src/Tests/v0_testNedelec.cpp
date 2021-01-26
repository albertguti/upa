
#include <iostream>
#include "structuredMesh.h"
#include "sparse_LIL.h"
#include "sparse_CSR.h"
#include "referenceElement.h"
#include "solver_CG.h"

using namespace std;
using namespace upa;

/** Basic setup for testing Nédelec FEM elements
 *
 *    We will solve
 *        curl curl E + c E = f     on Omega
 *        E x n = 0                 on delta Omega
 *
 *    which can be used to find diverge-free solutions of the wave equation.
 *    Remark: The selected boundary conditions are imposed automatically by Nedelec elements.
 *    Follows closely https://www.dealii.org/reports/nedelec/nedelec.pdf
 */
int main() {

    double source[2] = {1.0,1.0}; // f
    double velocity = 1.0; // c

    int dim = 2;
    ElemType type = ElemType::Triangle;

    StructuredMesh* mesh = new StructuredMesh();
    mesh->produceCartesian(dim,3,type);

    ReferenceElement* refElem = getReferenceElement(type,BFType::Nedelec,1);
    ReferenceElement* refElemAux = getReferenceElement(type,BFType::Lagrangian,1); // For the coordinate change


    int nE = mesh->getNumElements();
    int nN = mesh->getNumNodes();
    int nNbors = mesh->getNumElemNbors();

    int nG = refElem->getNumGaussPoints();
    double* gW = refElem->getGaussWeights();
    double* gC = refElem->getGaussCoords();
    double* bf = refElem->getBasisFunctions(0);
    double* dbf = refElem->getBasisFunctions(1);
    double* curlbf = refElem->getCurlBF();

    double* dbf_aux = refElemAux->getBasisFunctions(1);

    auto sysK = new Sparse_LIL(nN);
    double sysF[nN]; for(int i = 0; i < nN; ++i) sysF[i] = 0.0;
    double Area = 0.0;

    for (int e = 0; e < nE; ++e) { // Loop in elements
        int nodes[nNbors];
        double nodeCoords[nNbors*dim];
        mesh->getElemNodes(e,nodes);
        mesh->getElemCoords(e,nodeCoords);

        double Ke[nNbors*nNbors];
        double fe[nNbors];
        for (int i = 0; i < nNbors; ++i) {
            for (int j = 0; j < nNbors; ++j) Ke[i*nNbors+j] = 0.0;
            fe[i] = 0.0;
        }

        for (int k = 0; k < nG; ++k) { // Loop in Gauss Points

            /// Get data for this gpoint (reference coordinates)
            double wk = gW[k];
            double gCk[dim]; for (int i = 0; i < dim; ++i) gCk[i] = gC[dim*k+i];
            double bfk[nNbors*dim]; for (int i = 0; i < nNbors*dim; ++i) bfk[i] = bf[nNbors*dim*k+i];
            double dbfk[nNbors*dim*dim]; for (int i = 0; i < nNbors*dim*dim; ++i) dbfk[i] = dbf[nNbors*dim*dim*k+i];
            double curlbfk[nNbors]; for (int i = 0; i < nNbors; ++i) curlbfk[i] = curlbf[nNbors*k+i];

            double dbfk_aux[nNbors*dim]; for (int i = 0; i < nNbors*dim; ++i) dbfk_aux[i] = dbf_aux[nNbors*dim*k+i];

            /// Change to physical coordinates
            double J[dim*dim]; // Jacobian
            for (int i = 0; i < dim; ++i)
                for (int j = 0; j < dim; ++j) {
                    J[i * dim + j] = 0.0;
                    for (int l = 0; l < nNbors; ++l)
                        J[i * dim + j] += dbfk_aux[dim * l + i] * nodeCoords[dim * l + j];
                }

            double Jinv[dim*dim]; inverse(dim, J, Jinv);
            double detJ = det(dim,J);

            double bfk_xy[nNbors*dim]; // Nedelec basis functions in physical coordinates.
            double curlbfk_xy[nNbors]; // Curl in physical coordinates.
            for (int l = 0; l < nNbors; ++l) {
                curlbfk_xy[l] = curlbfk[l]/detJ;
                for (int i = 0; i < dim; ++i) {
                    bfk_xy[l*dim + i] = 0.0;
                    for (int j = 0; j < dim; ++j) {
                        bfk_xy[l*dim + i] += Jinv[i*dim + j] * bfk[l*dim + j];
                    }
                }
            }

            double dV = wk * detJ;
            Area += dV;

            /// Integration
            // Elemental matrix
            for (int i = 0; i < nNbors; ++i) {
                for (int j = 0; j < nNbors; ++j) {
                    Ke[i*nNbors+j] += curlbfk_xy[i] * curlbfk_xy[j] * dV;
                    for (int l = 0; l < dim; ++l) Ke[i*nNbors+j] += velocity * bfk_xy[i*dim+l] * bfk_xy[j*dim+l] * dV;
                }
            }
            // Elemental vector
            for (int i = 0; i < nNbors; ++i) {
                for (int l = 0; l < dim; ++l) fe[i] += bfk_xy[i*dim+l] * source[l] * dV;
            }

        }

        /// Assemble
        for (int i = 0; i < nNbors; ++i) {
            for (int j = 0; j < nNbors; ++j) {
                sysK->assemble(nodes[i], nodes[j], Ke[i*nNbors+j]);
            }
            sysF[nodes[i]] += fe[i];
        }
    }

    /// Solve system
    // Convert to CSR
    auto sysK_solve = new Sparse_CSR(sysK);

    cout << "Complete matrix: " << endl;
    for (int i = 0; i < nN; ++i) {
        for (int j = 0; j < nN ; ++j) {
            cout << "    " << sysK_solve->operator()(i,j);
        }
        cout << endl;
    }
    cout << endl;

    cout << "Complete vector: " << endl;
    for (int i = 0; i < nN; ++i) {
        cout << "    " << sysF[i];
    }
    cout << endl;

    // Create solver
    auto CG = new Solver_CG(nN,sysK_solve,sysF);
    CG->setIterations(1000);
    CG->setTolerance(0.000001);
    CG->setVerbosity(1);

    // Solve
    double x0[nN];
    for (int i = 0; i < nN; ++i) x0[i] = 0.0;
    CG->solve(x0);

    // Output
    double sol[nN];
    CG->getSolution(sol);
    cout << "Converged : " << CG->getConvergence() << endl;
    cout << "Number of iterations: " << CG->getNumIter() << endl;
    cout << "Error2 : " << CG->getError() << endl;

    cout << "x = " << endl;
    for (int i = 0; i < nN; ++i) {
        cout << sol[i] << " , ";
    }
    cout << endl;

}