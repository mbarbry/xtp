
#include <votca/xtp/bulkesp.h>
#include <votca/xtp/aomatrix.h>
#include <votca/tools/linalg.h>
//#include <boost/progress.hpp>
#include <votca/xtp/numerical_integrations.h>
#include <math.h> 
#include <votca/tools/constants.h>

#include "votca/xtp/orbitals.h"

using namespace votca::tools;


namespace votca { namespace xtp {
    namespace ub = boost::numeric::ublas;
    
    std::vector<Bulkesp::Molecule> Bulkesp::BreakIntoMolecules(std::vector< QMAtom* > _atoms, double scale){
        
        std::vector<Bulkesp::Molecule> mols;
        std::list<Bulkesp::Bond> bonds;
        
        Elements _elements;
        
        LOG(logDEBUG, *_log) << " BreakIntoMolecules(): locating bonds.\n" << flush;
        
        //find all the bonds;
        for (vector<QMAtom*>::iterator i = _atoms.begin(); i != _atoms.end(); ++i){
            for(vector<QMAtom*>::iterator j = i+1; j != _atoms.end(); ++j){
                double dif[3];
                dif[0] = ((*i)->x - (*j)->x);
                dif[1] = ((*i)->y - (*j)->y);
                dif[2] = ((*i)->z - (*j)->z);
                cout<<"dif: "<<dif[0]<<" "<<dif[1]<<" "<<dif[2]<<"\t";
                for(int k=0; k<3; k++){
                    if(periodic && std::abs(dif[k])>boxLen[k]*0.5) //correct for for bond crossing PBC, if it exists
                        if(dif[k]>0)    //i.x>j.x
                            dif[k]-=boxLen[k];
                        else            //i.x<j.x
                            dif[k]+=boxLen[k];
                }
                cout<<"PBC corrected: "<<dif[0]<<" "<<dif[1]<<" "<<dif[2]<<"\n";
                vec v(dif);
                double distSq = v*v;
                
                double acceptDist=_elements.getCovRad((*i)->type) + _elements.getCovRad((*j)->type);
                if(distSq<=acceptDist*acceptDist*scale){
                    Bond nb;        //bond goes
                    nb.a=(*i);      //from a
                    nb.b=(*j);      //to b
                    nb.ba=v;        //and has a (PBC corrected) vector ba (from b to a))
                    nb.a_indx=i-_atoms.begin();
                    nb.b_indx=j-_atoms.begin();
                    bonds.push_back(nb);
                }
            }
        }
        LOG(logDEBUG, *_log) << " BreakIntoMolecules(): "<< bonds.size() <<" bonds found.\n" << flush;
//        for (std::list<Bond>::iterator b = bonds.begin(); b != bonds.end(); ++b){
//            LOG(logDEBUG, *_log) << b->a_indx <<" - "<<b->b_indx<<"\t"<<b->ba<<"\t"<<sqrt(b->ba*b->ba)<< flush;
//        }
  
        
        //now add all the atoms that have no bonds as a separate molecule
        Bulkesp::Molecule leftover;
        for (vector<QMAtom*>::iterator i = _atoms.begin(); i != _atoms.end(); ++i){
            bool found = false; //does this atom have bonds?
            for (std::list<Bond>::iterator b = bonds.begin(); b != bonds.end(); ++b){
                if(b->a==*i || b->b==*i){
                    found=true;
                    break;
                }
            }
            if(!found) //atom has no bonds
                leftover.atoms.push_back(*i);
                leftover.atomIndeces.push_back(i-_atoms.begin());
        }
        if(leftover.atoms.size()>0){
            mols.push_back(leftover);
            LOG(logDEBUG, *_log) << " BreakIntoMolecules(): put "<< leftover.atoms.size() 
                    << " Unbonded atoms into molecule "<< mols.size() -1 <<";\t" << bonds.size() <<" bonds left.\n" << flush;
        }
        
        
        
        //assign bonds to molecules
        while(bonds.size()>0){
            //init the new molecule with the last bond we have
            Bulkesp::Molecule m;
            std::list<Bond>::iterator last = bonds.end();
            std::advance(last, -1);
            m.atoms.push_back(last->a);
            m.atomIndeces.push_back(last->a_indx);
            leftover.atomIndeces.push_back(bonds.end()->a_indx);
            if(periodic){//unwrap system by moving b
                last->b->x = last->a->x - last->ba.getX();
                last->b->y = last->a->y - last->ba.getY();
                last->b->z = last->a->z - last->ba.getZ();
            }
            m.atoms.push_back(last->b);
            m.atomIndeces.push_back(last->b_indx);
            bonds.pop_back();
            
            
            int oldMsize;
            do{
                oldMsize=m.atoms.size();
                //loop through bonds to see what binds to what is already in the molecule
                for (std::list<Bond>::iterator b = bonds.begin(); b != bonds.end(); ++b){
                    if(find (m.atoms.begin(), m.atoms.end(), b->a)!=m.atoms.end()){ //molecule contains a
                        //double check if molecule contains b already (don't double count atoms in circular molecules)
                        if(find (m.atoms.begin(), m.atoms.end(), b->b)==m.atoms.end()){ // contains a, but not b
                            if(periodic){//unwrap system by moving b
                                b->b->x = b->a->x - b->ba.getX();
                                b->b->y = b->a->y - b->ba.getY();
                                b->b->z = b->a->z - b->ba.getZ();
                            }
                            m.atoms.push_back(b->b); //add b to molecule
                            m.atomIndeces.push_back(b->b_indx);
                            b=bonds.erase(b);
                            --b;
                        }
                    }
                    else if(find (m.atoms.begin(), m.atoms.end(), b->b)!=m.atoms.end()){ //molecule contains b
                        //double check if molecule contains a already (don't double count atoms in circular molecules)
                        if(find (m.atoms.begin(), m.atoms.end(), b->a)==m.atoms.end()){ // contains b, but not a
                            if(periodic){//unwrap system by moving a
                                b->a->x = b->b->x + b->ba.getX();
                                b->a->y = b->b->y + b->ba.getY();
                                b->a->z = b->b->z + b->ba.getZ();
                            }
                            m.atoms.push_back(b->a); //add a to molecule
                            m.atomIndeces.push_back(b->a_indx);
                            b=bonds.erase(b);
                            --b;
                        }
                    }

                }
            }while(oldMsize!=m.atoms.size());   //keep looping until nothing is added to m
            
            //we are done with this molecule, save it to mols
            mols.push_back(m);
            LOG(logDEBUG, *_log) << " BreakIntoMolecules(): put "<< m.atoms.size() 
                    << " atoms into molecule "<< mols.size() -1 <<";\t" << bonds.size() <<" bonds left.\n" << flush;
        }
        
        if(periodic)
            LOG(logDEBUG, *_log) << " BreakIntoMolecules(): Molecules have been unwrapped.\n" << flush;
        
        return(mols);
    }
    
    
    
    ub::matrix<double> Bulkesp::BuildDenMat(Orbitals &_orb, std::string _state, std::string _spin, int _state_no)
    {
        ub::matrix<double> DMAT_tot;
        bool _do_transition=false;
        
        if(_state!="ground")
            LOG(logDEBUG, *_log) << " Bulkesp is not tested for any state other than the ground state. "
                    <<"It will likely not work correctly, as BSE Singlet and Triplet Coefficients are simply copied from the global orbital object.\n"  << flush; 
        
        if(_state=="transition"){
            _do_transition=true;
            if (_spin=="singlet"){
                DMAT_tot=_orb.TransitionDensityMatrix(_orb.MOCoefficients(), _orb.BSESingletCoefficients(), _state_no-1);
            }
            else if (_spin=="triplet"){
                DMAT_tot=_orb.TransitionDensityMatrix(_orb.MOCoefficients(), _orb.BSETripletCoefficients(), _state_no-1); 
            }
            else throw std::runtime_error("Spin entry not recognized");
        }
        else if (_state=="ground" || _state=="excited"){
            
        
            ub::matrix<double> &DMATGS=_orb.DensityMatrixGroundState(_orb.MOCoefficients());
            DMAT_tot=DMATGS;
            if ( _state_no > 0 && _state=="excited"){
                std::vector<ub::matrix<double> > DMAT;
                if (_spin=="singlet"){
                    DMAT = _orb.DensityMatrixExcitedState(_orb.MOCoefficients() , _orb.BSESingletCoefficients(), _state_no-1);
                }
                else if (_spin=="triplet"){
                    DMAT = _orb.DensityMatrixExcitedState( _orb.MOCoefficients() , _orb.BSETripletCoefficients(), _state_no-1);
                }
                else throw std::runtime_error("Spin entry not recognized");
                DMAT_tot=DMAT_tot-DMAT[0]+DMAT[1];
            }            
	   // Ground state + hole_contribution + electron contribution
	}
        else throw std::runtime_error("State entry not recognized");
        
        
        return(DMAT_tot);
    }
    
    
    
    
    ub::matrix<double> Bulkesp::BuildOverlapMat(Orbitals &_molOrb, Orbitals &_globalOrb){
        ub::matrix<double> ov;
        return (ov);
    }
    
    
    
    /**
     * 
     * @param _global_atomlist
     * @param _local_atomlist
     * @param _local_atomIndeces Indeces of atoms in current molecule
     * @param _global_dmat
     * @param _global_basis
     * @param bs
     * @param gridsize
     * @param _grid
     * @return 
     */
    ub::vector<double> Bulkesp::ComputeESP(std::vector< QMAtom* > & _global_atomlist,
            std::vector< QMAtom* > & _local_atomlist, std::vector<int> _local_atomIndeces,
            ub::matrix<double> &_global_dmat, AOBasis &_global_basis, BasisSet &bs, string gridsize, Grid &_grid, double &netcharge){
        

        ub::vector<double> _ESPatGrid = ub::zero_vector<double>(_grid.getsize());
        
        NumericalIntegration numway;

        numway.GridSetup(gridsize,&bs,_global_atomlist);
        LOG(logDEBUG, *_log) << TimeStamp() << " Calculate Densities at Numerical Grid with gridsize "<<gridsize  << flush; 
        //As long as basis functions are well supported and molecules are smaller than 0.5*boxLen along any axis, then
        //density integration should be accurate enough without making it explicitly periodic
        double N=numway.IntegrateDensity_Molecule(_global_dmat,&_global_basis,_local_atomIndeces);
        LOG(logDEBUG, *_log) << TimeStamp() << " Calculated Densities at Numerical Grid, Number of electrons is "<< N << flush; 


        netcharge=getNetcharge( _local_atomlist,N );   

        LOG(logDEBUG, *_log) << TimeStamp() << " Calculating ESP at CHELPG grid points"  << flush;     
        //boost::progress_display show_progress( _grid.getsize() );
        
        if(periodic){
            LOG(logDEBUG, *_log) << " Bulkesp::ComputeESP(): periodicity is on, including long range contributions."<< endl;
            double BL[3];
            BL[0]=boxLen[0]*tools::conv::ang2bohr;
            BL[1]=boxLen[1]*tools::conv::ang2bohr;
            BL[2]=boxLen[2]*tools::conv::ang2bohr;
            
            double exactMadelung=1.74756459463318;
            
            int natomsonside=12;
            double numK=26;
            double a = 5.6402*0.5*natomsonside*tools::conv::ang2bohr;
            //a*= 1.14;
            cout<< "a = "<< a << endl;
            cout<< "nearest neighbour distance = "<< a/8.0 << endl;
            BL[0]=a;
            BL[1]=a;
            BL[2]=a;
            
            //numway.PrepKspaceDensity(BL, 1.6);
            numway.PrepKspaceDensity(BL, a/numK, natomsonside);
            LOG(logDEBUG, *_log) << " Bulkesp::ComputeESP(): Found density in Fourier space."<< endl;
            //#pragma omp parallel for
            for ( int i = 0 ; i < _grid.getsize(); i++){
                //_ESPatGrid(i)=numway.IntegratePotential_w_PBC(_grid.getGrid()[i]*tools::conv::nm2bohr, BL);
                
                ub::vector<double> madelungPoint;
                madelungPoint.resize(3);
//                madelungPoint(0)=a/2;
//                madelungPoint(1)=a/2;
//                madelungPoint(2)=a/2;
                madelungPoint(0)=0.0;
                madelungPoint(1)=0.0;
                madelungPoint(2)=0.0;
                _ESPatGrid(i)=numway.IntegratePotential_w_PBC(madelungPoint, BL);
                cout<<"Madelung constant is: "<< _ESPatGrid(i)*(a/natomsonside) <<endl;
                exit(-1);
                //++show_progress;
            }
            numway.FreeKspace();
        }
        else{
            LOG(logDEBUG, *_log) << " Bulkesp::ComputeESP(): periodicity is off, no long range contributions."<< endl;
            #pragma omp parallel for
            for ( int i = 0 ; i < _grid.getsize(); i++){
                _ESPatGrid(i)=numway.IntegratePotential(_grid.getGrid()[i]*tools::conv::nm2bohr);
                //++show_progress;
            }
        }
        LOG(logDEBUG, *_log) << TimeStamp() << " Electron contribution calculated"  << flush; 
        
        // Calculating nuclear potential at gridpoints
        ub::vector<double> _NucPatGrid = EvalNuclearPotential(  _local_atomlist,  _grid );
        _ESPatGrid += _NucPatGrid;
        LOG(logDEBUG, *_log) << TimeStamp() << " Nuclear contribution calculated"  << flush; 

        
        return(_ESPatGrid);
    }
    
    
    
    void Bulkesp::FillElement2NBF(std::vector< QMAtom* >& _atomlist, BasisSet &bs){
        
        //Find list of elements
        _elements.clear();
        for (std::vector< QMAtom* >::iterator ait = _atomlist.begin(); ait < _atomlist.end(); ait++) {

            std::string element_name = (*ait)->type;
            list<std::string>::iterator ite;
            ite = find(_elements.begin(), _elements.end(), element_name);
            if (ite == _elements.end()) {            //this is the first atom of this element encountered
                _elements.push_back(element_name);
            }
        }
        
        //Find NBF for each element
        _element2NBF.clear();
        for(list<std::string>::iterator eit = _elements.begin(); eit!=_elements.end(); ++eit){
            vector<Shell*> shells = bs.getElement(*eit)->getShells();
            int nfunc=0;
            //loop over shells
            for(vector< Shell* >::iterator sit = shells.begin(); sit!=shells.end(); ++sit){
                nfunc += 1 + 2 * ((*sit)->getLmax());
            }
            //store number of functions
            _element2NBF[(*eit)]=nfunc;
        }
        
    }
    
    
    
    //Find indeces of MO coefficients of the first shell of each atom,
    //assuming that the order of _atomlist is the same as that of the _MO_Coefficients.
    std::map<QMAtom*,int> Bulkesp::MapAtom2MOCoefIndex(std::vector< QMAtom* >& _atomlist){
            std::map<QMAtom*,int> ret;
            int count=0;
            //loop over all atoms in system
            for(vector<QMAtom*>::iterator i = _atomlist.begin(); i != _atomlist.end(); ++i){
                ret[(*i)] = count;
                count += _element2NBF[(*i)->type];
            }
            return(ret);
    }
    
    
                
    void Bulkesp::Evaluate(std::vector< QMAtom* >& _atomlist, ub::matrix<double> &_global_dmat, Orbitals& _globalOrb,
            ub::matrix<double> _global_MO_Coeffs, AOBasis &_basis, BasisSet &bs,
            string gridsize, double maxBondScale, std::string _state,
            std::string _spin, int _state_no){
        
        //find the individual molecules
        std::vector<Bulkesp::Molecule> mols = BreakIntoMolecules(_atomlist, maxBondScale);
        
        //loop over molecules
        LOG(logDEBUG, *_log) << " Bulkesp::Evaluate(): found "<< mols.size() << "molecules.\n" << flush; 
        for (std::vector<Bulkesp::Molecule>::iterator m = mols.begin(); m != mols.end(); ++m){
            
            LOG(logDEBUG, *_log) << " Bulkesp::Evaluate(): "<< TimeStamp()<<" processing molecule "<< m-mols.begin() << endl; 
			
            //veryfy atomic coordinates and units
            for(std::vector<QMAtom*>::iterator a = m->atoms.begin(); a != m->atoms.end(); ++a){
                QMAtom* ap=*a;
                cout << ap->type << '\t' << ap->x << '\t' << ap->y << '\t' << ap->z << endl;
            }
			cout << "box: " << boxLen[0] << '\t' << boxLen[0] << '\t'<< boxLen[0] << endl;
			
            //set up grid
            Grid _grid(true,false,false); //create polarsites, so we can output grid to .cube file
//            _grid.setAtomlist(&m->atoms);
//            _grid.setupCHELPgrid();
            _grid.setAtomlist(&_atomlist);
            _grid.setCubegrid(true);
            _grid.setupgrid();
            LOG(logDEBUG, *_log) << TimeStamp() <<  " Done setting up CHELPG grid with " << _grid.getsize() << " points " << endl;
            
            //calculate the ESP
            //ub::vector<double> ESP=ComputeESP(m->atoms, _m_dmat, _m_ovmat, _m_basis, bs, gridsize, _grid);
            double netcharge=0.0;
            ub::vector<double> ESP = ComputeESP(_atomlist, m->atoms, m->atomIndeces,
                                                _global_dmat, _basis, bs, gridsize, _grid, netcharge);
                
            //output
            //CHELPG grids aren't periodic and equally spaced,
            //so can't output to .cube format.
            //Create own format.
            std::ostringstream fn;
            fn << "BulkEsp_" << m-mols.begin() << ".grid";
            _grid.writeIrregularGrid(fn.str(), ESP, m->atoms, periodic, boxLen);
            
            
            //store the potential in apolarsites
            for ( int i = 0 ; i < _grid.getsize(); i++){
                _grid.Sites()[i]->setPhi(ESP(i), 0.0);
            }
            //and save it to a .cube file
//            fn.str(std::string());
//            fn << "BulkEsp_" << m-mols.begin() << ".cube";
//            _grid.printgridtoCubefile(fn.str());
            
            //TODO: fit charges
            std::vector< ub::vector<double> > _fitcenters;
    
            for ( unsigned j = 0; j < m->atoms.size(); j++){
               ub::vector<double> _pos(3);
              _pos(0) = tools::conv::ang2nm*(m->atoms[j]->x);
              _pos(1) = tools::conv::ang2nm*(m->atoms[j]->y);
              _pos(2) = tools::conv::ang2nm*(m->atoms[j]->z);
              _fitcenters.push_back(_pos);            
            }

            std::vector<double> _charges = FitPartialCharges(_fitcenters,_grid, ESP, netcharge);

            //Write charges to qmatoms
            for ( unsigned _i =0 ; _i < m->atoms.size(); _i++){
                m->atoms[_i]->charge=_charges[_i];
            } 
            
        }
        LOG(logDEBUG, *_log) << " Bulkesp::Evaluate(): "<< TimeStamp()<<" All molecules processed." << endl << flush; 
        
    }
    
    
}}