#include "dilep_producer.hpp"
#include "utilities.hpp"
#include "KinZfitter.hpp"

#include "TLorentzVector.h"

using namespace std;

DileptonProducer::DileptonProducer(int year_){
    year = year_;
    kinZfitter = new KinZfitter();

  //This variable is to debug how often the kinematic refitting code is used
    cnt_refit = 0;

}

DileptonProducer::~DileptonProducer(){
    kinZfitter = new KinZfitter();

  //This variable is to debug how often the kinematic refitting code is used
    cnt_refit = 0;

}

void DileptonProducer::WriteDileptons(pico_tree &pico, 
                                      std::vector<int> sig_el_pico_idx, std::vector<int> sig_mu_pico_idx) {
  pico.out_nll() = 0;
  if (pico.out_nmu() < 2 && pico.out_nel() < 2) return;
  double mindm(999), zmass(91.1876);
  int nll(0), shift(0);
  TLorentzVector l1err, l2err;
  double ptl1err, ptl2err;
  double dml1, dml2;
  int idx_fsr1, idx_fsr2;
  //KinZfitter::KinZfitter kinZfitter= KinZfitter();
  //KinZfitter *kinZfitter = new KinZfitter();
  TLorentzVector ll_refit(0,0,0,0);
  std::vector<TLorentzVector> refit_leptons{ll_refit,ll_refit};
  std::map<unsigned int, TLorentzVector> leptons_map;
  std::map<unsigned int, double> leptons_pterr_map;
  std::map<unsigned int, TLorentzVector> fsrphotons_map;
  TLorentzVector fsrphoton1, fsrphoton2;
  int status, covmatstatus = -5;
  float minnll = -5;


  if (pico.out_nmu()>=2)
    for(size_t i(0); i < sig_mu_pico_idx.size(); i++) 
      for(size_t j(i+1); j < sig_mu_pico_idx.size(); j++) {
        int imu1 = sig_mu_pico_idx.at(i);
        int imu2 = sig_mu_pico_idx.at(j);
        TLorentzVector mu1, mu2, dimu;
        fsrphoton1 = TLorentzVector(0,0,0,0); 
        fsrphoton2 = TLorentzVector(0,0,0,0);

        mu1.SetPtEtaPhiM(pico.out_mu_pt()[imu1], pico.out_mu_eta()[imu1],
                         pico.out_mu_phi()[imu1], 0.10566);
        mu2.SetPtEtaPhiM(pico.out_mu_pt()[imu2], pico.out_mu_eta()[imu2],
                         pico.out_mu_phi()[imu2], 0.10566);
        dimu = mu1 + mu2;
        // Dilepton closest to Z mass gets put at the front
        if(abs(dimu.M() - zmass) < mindm) { 
          mindm = abs(dimu.M() - zmass);
          shift = 0;
        }
        else
          shift = nll;

        ptl1err = pico.out_mu_ptErr()[imu1];
        ptl2err = pico.out_mu_ptErr()[imu2];
        l1err.SetPtEtaPhiM(pico.out_mu_pt()[imu1] + ptl1err,
                           pico.out_mu_eta()[imu1], pico.out_mu_phi()[imu1], 0.10566);
        l2err.SetPtEtaPhiM(pico.out_mu_pt()[imu2] + ptl2err,
                           pico.out_mu_eta()[imu2], pico.out_mu_phi()[imu2], 0.10566);
        dml1 = (l1err + mu2).M() - dimu.M();
        dml2 = (mu1 + l2err).M() - dimu.M();

        leptons_map[0] = mu1;
        leptons_map[1] = mu2;
        leptons_pterr_map[0] = ptl1err;
        leptons_pterr_map[1] = ptl2err;
       
        //Loops through FSRphotons to find which leptons are associated with to be used for constrained fit
        idx_fsr1 = -1; idx_fsr2 = -1;
        for(size_t idx_fsr = 0; idx_fsr < static_cast<unsigned int>(pico.out_nfsrphoton()); idx_fsr++){
            if(pico.out_fsrphoton_muonidx()[idx_fsr]==imu1){
              if(idx_fsr1 != -1 && (pico.out_fsrphoton_droveret2()[idx_fsr1] <  pico.out_fsrphoton_droveret2()[idx_fsr])){ continue;}
              idx_fsr1 = idx_fsr; continue;
            }
            if(pico.out_fsrphoton_muonidx()[idx_fsr]==imu2){
              if(idx_fsr2 != -1 && (pico.out_fsrphoton_droveret2()[idx_fsr2] <  pico.out_fsrphoton_droveret2()[idx_fsr])){ continue;}
              idx_fsr2 = idx_fsr; continue;
            }
        }
        fsrphotons_map.clear();
        int fsr_cnt = 0;
        if(idx_fsr1!=-1){
          fsrphoton1.SetPtEtaPhiM( pico.out_fsrphoton_pt()[idx_fsr1],pico.out_fsrphoton_eta()[idx_fsr1],pico.out_fsrphoton_phi()[idx_fsr1],0 );
          fsrphotons_map[fsr_cnt] = fsrphoton1;
          fsr_cnt++;
        }
        if(idx_fsr2!=-1){
          fsrphoton2.SetPtEtaPhiM( pico.out_fsrphoton_pt()[idx_fsr2],pico.out_fsrphoton_eta()[idx_fsr2],pico.out_fsrphoton_phi()[idx_fsr2],0 ); 
          fsrphotons_map[fsr_cnt] = fsrphoton2;     
        }

        //Code Executing the kinematic refit
        if(pico.out_nphoton() > 0){
          kinZfitter->Setup(leptons_map, fsrphotons_map, leptons_pterr_map);
          kinZfitter->KinRefitZ1();
          refit_leptons = kinZfitter->GetRefitP4s();
          ll_refit     = refit_leptons[0] + refit_leptons[1];
          status       = kinZfitter -> GetStatus();
          covmatstatus = kinZfitter -> GetCovMatStatus();
          minnll       = kinZfitter -> GetMinNll();

          //debug statement
          cnt_refit++; std::cout << cnt_refit << std::endl;
        } else {
        
          ll_refit = TLorentzVector(0,0,0,0);
        }

        pico.out_nll()++;
        pico.out_ll_pt()   .insert(pico.out_ll_pt()   .begin()+shift, dimu.Pt());
        pico.out_ll_eta()  .insert(pico.out_ll_eta()  .begin()+shift, dimu.Eta());
        pico.out_ll_phi()  .insert(pico.out_ll_phi()  .begin()+shift, dimu.Phi());
        pico.out_ll_m()    .insert(pico.out_ll_m()    .begin()+shift, dimu.M());
        pico.out_ll_dr()   .insert(pico.out_ll_dr()   .begin()+shift, mu1.DeltaR(mu2));
        pico.out_ll_dphi() .insert(pico.out_ll_dphi() .begin()+shift, fabs(mu1.DeltaPhi(mu2)));
        pico.out_ll_deta() .insert(pico.out_ll_deta() .begin()+shift, fabs(mu1.Eta()-mu2.Eta()));
        pico.out_ll_lepid().insert(pico.out_ll_lepid().begin()+shift, 13);
        pico.out_ll_i1()   .insert(pico.out_ll_i1()   .begin()+shift, imu1);
        pico.out_ll_i2()   .insert(pico.out_ll_i2()   .begin()+shift, imu2);
        pico.out_ll_l1_masserr() .insert(pico.out_ll_l1_masserr() .begin()+shift, dml1);
        pico.out_ll_l2_masserr() .insert(pico.out_ll_l2_masserr() .begin()+shift, dml2);
        pico.out_ll_charge()     .insert(pico.out_ll_charge()     .begin()+shift, pico.out_mu_charge()[imu1]+pico.out_mu_charge()[imu2]);

        //Assigns the refit lepton/dilepton quantities
        pico.out_ll_refit_pt()   .insert(pico.out_ll_refit_pt()   .begin()+shift, ll_refit.Pt());
        pico.out_ll_refit_eta()  .insert(pico.out_ll_refit_eta()  .begin()+shift, ll_refit.Eta());
        pico.out_ll_refit_phi()  .insert(pico.out_ll_refit_phi()  .begin()+shift, ll_refit.Phi());
        pico.out_ll_refit_m()    .insert(pico.out_ll_refit_m()    .begin()+shift, ll_refit.M());
        pico.out_ll_refit_l1_pt().insert(pico.out_ll_refit_l1_pt().begin()+shift, refit_leptons[0].Pt());
        pico.out_ll_refit_l2_pt().insert(pico.out_ll_refit_l2_pt().begin()+shift, refit_leptons[1].Pt());

        pico.out_ll_refit_status()   .insert(pico.out_ll_refit_status()   .begin()+shift, status);
        pico.out_ll_refit_covmat_status()   .insert(pico.out_ll_refit_covmat_status()   .begin()+shift, covmatstatus);
        pico.out_ll_refit_minnll()   .insert(pico.out_ll_refit_minnll()   .begin()+shift, minnll);

        nll++;
      }
  fsrphotons_map.clear();
  if (pico.out_nel()>=2)
    for(size_t i(0); i < sig_el_pico_idx.size(); i++) 
      for(size_t j(i+1); j < sig_el_pico_idx.size(); j++) {
        int iel1 = sig_el_pico_idx.at(i);
        int iel2 = sig_el_pico_idx.at(j);
        TLorentzVector el1, el2, diel;

        el1.SetPtEtaPhiM(pico.out_el_pt()[iel1] ,pico.out_el_eta()[iel1],
                         pico.out_el_phi()[iel1],0.000511);
        el2.SetPtEtaPhiM(pico.out_el_pt()[iel2], pico.out_el_eta()[iel2],
                         pico.out_el_phi()[iel2],0.000511);
        diel = el1 + el2;
        // Dilepton closest to Z mass (not overlapping with leading signal photon) gets put at the front
        bool ph_el_overlap = (pico.out_nphoton() > 0 && (static_cast<int>(i)==pico.out_photon_elidx()[0] || static_cast<int>(j)==pico.out_photon_elidx()[0])) || (pico.out_nphoton()==0); //checks for overlap
        if(abs(diel.M() - zmass) < mindm && ph_el_overlap) {
          mindm = abs(diel.M() - zmass);
          shift = 0;
        } 
        else
          shift = nll;

        ptl1err = pico.out_el_energyErr()[iel1] * el1.Pt() / el1.P();
        ptl2err = pico.out_el_energyErr()[iel2] * el2.Pt() / el2.P();
        l1err.SetPtEtaPhiM(pico.out_el_pt()[iel1] + ptl1err,
                           pico.out_el_eta()[iel1], pico.out_el_phi()[iel1], 0.0005);
        l2err.SetPtEtaPhiM(pico.out_el_pt()[iel2] + ptl2err,
                           pico.out_el_eta()[iel2], pico.out_el_phi()[iel2], 0.0005);
        dml1 = (l1err + el2).M() - diel.M();
        dml2 = (el1 + l2err).M() - diel.M();


        leptons_map[0] = el1;
        leptons_map[1] = el2;
        leptons_pterr_map[0] = ptl1err;
        leptons_pterr_map[1] = ptl2err;

        //Code executing kinematic refit
        if(pico.out_nphoton() > 0){
          kinZfitter->Setup(leptons_map, fsrphotons_map, leptons_pterr_map);
          kinZfitter->KinRefitZ1();
          refit_leptons = kinZfitter->GetRefitP4s();
          ll_refit = refit_leptons[0] + refit_leptons[1];
          status       = kinZfitter -> GetStatus();
          covmatstatus = kinZfitter -> GetCovMatStatus();
          minnll       = kinZfitter -> GetMinNll();

          //debug statement
          cnt_refit++; std::cout << cnt_refit << std::endl;

        } else {
        
          ll_refit = TLorentzVector(0,0,0,0);
        }

        pico.out_nll()++;
        pico.out_ll_pt()   .insert(pico.out_ll_pt()   .begin()+shift, diel.Pt());
        pico.out_ll_eta()  .insert(pico.out_ll_eta()  .begin()+shift, diel.Eta());
        pico.out_ll_phi()  .insert(pico.out_ll_phi()  .begin()+shift, diel.Phi());
        pico.out_ll_m()    .insert(pico.out_ll_m()    .begin()+shift, diel.M());
        pico.out_ll_dr()   .insert(pico.out_ll_dr()   .begin()+shift, el1.DeltaR(el2));
        pico.out_ll_dphi() .insert(pico.out_ll_dphi() .begin()+shift, fabs(el1.DeltaPhi(el2)));
        pico.out_ll_deta() .insert(pico.out_ll_deta() .begin()+shift, fabs(el1.Eta()-el2.Eta()));
        pico.out_ll_lepid().insert(pico.out_ll_lepid().begin()+shift, 11);
        pico.out_ll_i1()   .insert(pico.out_ll_i1()   .begin()+shift, iel1);
        pico.out_ll_i2()   .insert(pico.out_ll_i2()   .begin()+shift, iel2);
        pico.out_ll_l1_masserr() .insert(pico.out_ll_l1_masserr() .begin()+shift, dml1);
        pico.out_ll_l2_masserr() .insert(pico.out_ll_l2_masserr() .begin()+shift, dml2);
        pico.out_ll_charge()     .insert(pico.out_ll_charge()     .begin()+shift, pico.out_el_charge()[iel1]+pico.out_el_charge()[iel2]);

        //Assigns the refit lepton/dilepton quantities
        pico.out_ll_refit_pt()   .insert(pico.out_ll_refit_pt()   .begin()+shift, ll_refit.Pt());
        pico.out_ll_refit_eta()  .insert(pico.out_ll_refit_eta()  .begin()+shift, ll_refit.Eta());
        pico.out_ll_refit_phi()  .insert(pico.out_ll_refit_phi()  .begin()+shift, ll_refit.Phi());
        pico.out_ll_refit_m()    .insert(pico.out_ll_refit_m()    .begin()+shift, ll_refit.M());
        pico.out_ll_refit_l1_pt().insert(pico.out_ll_refit_l1_pt().begin()+shift, refit_leptons[0].Pt());
        pico.out_ll_refit_l2_pt().insert(pico.out_ll_refit_l2_pt().begin()+shift, refit_leptons[1].Pt());

        pico.out_ll_refit_status()   .insert(pico.out_ll_refit_status()   .begin()+shift, status);
        pico.out_ll_refit_covmat_status()   .insert(pico.out_ll_refit_covmat_status()   .begin()+shift, covmatstatus);
        pico.out_ll_refit_minnll()   .insert(pico.out_ll_refit_minnll()   .begin()+shift, minnll);

        nll++;
      }

  //delete kinZfitter;
  return;
}
