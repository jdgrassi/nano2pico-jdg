#!/bin/env python
import os, argparse
import ROOT
import glob
import re

#SMS-T5qqqqZH_HToBB-mGluino-1000to1300-mLSP0to1100_TuneCUETP8M1_13TeV-madgraphMLM-pythia8__RunIISummer16NanoAODv7__PUSummer16v3Fast_Nano02Apr2020_GridpackScan_102X_mcRun2_asymptotic_v8-v1__*.root
def get_dataset_name(path):
  input_filename = os.path.basename(path)
  dataset_name = re.search('.*__[0-9]+?__', input_filename).group()
  dataset_name = re.sub('__[0-9]+?__', '', dataset_name)
  return dataset_name

if __name__ == '__main__':

  parser = argparse.ArgumentParser(description="Submits batch jobs to split mass points of signal NanoAOD files",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument("-i","--in_dir", default="/net/cms24/cms24r0/pico/NanoAODv7/nano/2016/SMS-T5qqqqZH_unsplit_fastSimJmeCorrection", required=True,
                      help="Directory where the unsplit NanoAOD files are")
  parser.add_argument("-m","--model", default="", required=True,
                      help="Model of signal: SMS-T5qqqqZH_HToBB-mGluino or SMS-T5qqqqZH_HToBB-mN2. Used for input glob and GenModel naming convention.")
  parser.add_argument("-t","--target_dir", default="/net/cms24/cms24r0/pico/NanoAODv7/nano/2016/SMS-T5qqqqZH_fastSimJmeCorrection", required=True,
                      help="Determines the output folder.")
  parser.add_argument("-o","--out_cmd_filename", default="cmds.py", required=True,
                      help="File with list of commands for batch system.")
  parser.add_argument("-e","--temp_entrylist_dir", default="split_entrylist",
                      help="Temporary folder for entrylist root files")
  args = parser.parse_args()

  # Print error if there is already a temp_entrylist_dir
  if not os.path.exists(args.temp_entrylist_dir): os.makedirs(args.temp_entrylist_dir)

  # Collect files according to dataset
  # dataset_files_dict[dataset] = [files]
  dataset_files_dict = {}
  input_files = glob.glob(args.in_dir+'/'+args.model+'*.root')
  for input_path in input_files:
    input_filename = os.path.basename(input_path)
    #print(input_filename)
    # Search for __00000__ to find dataset name
    #dataset_name = re.search('.*__[0-9]+?__', input_filename).group()
    #dataset_name = re.sub('__[0-9]+?__', '', dataset_name)
    dataset_name = get_dataset_name(input_path)
    if dataset_name not in dataset_files_dict: dataset_files_dict[dataset_name] = []
    dataset_files_dict[dataset_name].append(input_filename)
  #print(dataset_files_dict)
  print('Found dataset names:')
  for dataset_name in dataset_files_dict: print('  '+dataset_name)

  # Make entrylist root files for each dataset
  # Output: temp_entrylist_dir/dataset_name/split_entrylist_GenModel_xxx.root
  for dataset_name in dataset_files_dict:
    # root 'root_scripts/make_split_entrylists.cxx+("input_glob", "entrylist_directory", nlsp_pdgId, lsp_pdgId, apply_chi2_to_higgs_cut)'
    entrylist_dir = args.temp_entrylist_dir+'/'+dataset_name
    dataset_glob = args.in_dir+'/'+dataset_name+'*.root'
    ## For debugging
    #dataset_glob = "/net/cms24/cms24r0/pico/NanoAODv7/nano/2016/SMS-T5qqqqZH_unsplit_fastSimJmeCorrection/SMS-T5qqqqZH_HToBB-mGluino-1000to1300-mLSP0to1100_TuneCUETP8M1_13TeV-madgraphMLM-pythia8__RunIISummer16NanoAODv7__PUSummer16v3Fast_Nano02Apr2020_GridpackScan_102X_mcRun2_asymptotic_v8-v1__00000__2F9532EE-BC57-D349-8411-FD9FFEC5B571.root"
    #dataset_glob = "/net/cms24/cms24r0/pico/NanoAODv7/nano/2016/SMS-T5qqqqZH_unsplit_fastSimJmeCorrection/SMS-T5qqqqZH_HToBB-mN2-1000to1800_TuneCUETP8M1_13TeV-madgraphMLM-pythia8__RunIISummer16NanoAODv7__PUSummer16v3Fast_Nano02Apr2020_GridpackScan_102X_mcRun2_asymptotic_v8-v1__10000__EEF1A2CA-4794-EC45-BE9A-4CD629315C1C.root"
    if not os.path.exists(entrylist_dir): os.makedirs(entrylist_dir)
    command = 'root -q -l \'root_scripts/make_split_entrylists.cxx++("'+dataset_glob+'","'+entrylist_dir+'", 1)\''
    print('Running below command')
    print('  '+command)
    os.system(command)
    # Used for debugging
    #break

  # Associate root collection with entrylist
  # dataset_entrylists_dict[dataset] = [entrylist]
  dataset_entrylists_dict = {}
  entrylist_folders = glob.glob(args.temp_entrylist_dir+'/*')
  for entrylist_folder in entrylist_folders:
    dataset_name = os.path.basename(entrylist_folder)
    entrylist_files = glob.glob(args.temp_entrylist_dir+'/'+dataset_name+'/*.root')
    dataset_entrylists_dict[dataset_name] = entrylist_files

  # Make commands for spliting root files
  commands = []
  for dataset_name in dataset_files_dict:
    for entrylist_path in dataset_entrylists_dict[dataset_name]:
      #print(entrylist_path)
      entrylist_filename = os.path.basename(entrylist_path)
      entrylist_dir = os.path.dirname(entrylist_path)
      nlsp_mass = re.search('T5qqqqZH_[0-9]+_[0-9]+', entrylist_filename).group().split('_')[1]
      lsp_mass = re.search('T5qqqqZH_[0-9]+_[0-9]+', entrylist_filename).group().split('_')[2]
      dataset_glob = args.in_dir+'/'+dataset_name+'*.root'
      ## For debugging
      #dataset_glob = "/net/cms24/cms24r0/pico/NanoAODv7/nano/2016/SMS-T5qqqqZH_unsplit_fastSimJmeCorrection/SMS-T5qqqqZH_HToBB-mGluino-1000to1300-mLSP0to1100_TuneCUETP8M1_13TeV-madgraphMLM-pythia8__RunIISummer16NanoAODv7__PUSummer16v3Fast_Nano02Apr2020_GridpackScan_102X_mcRun2_asymptotic_v8-v1__00000__2F9532EE-BC57-D349-8411-FD9FFEC5B571.root"
      #dataset_glob = "/net/cms24/cms24r0/pico/NanoAODv7/nano/2016/SMS-T5qqqqZH_unsplit_fastSimJmeCorrection/SMS-T5qqqqZH_HToBB-mN2-1000to1800_TuneCUETP8M1_13TeV-madgraphMLM-pythia8__RunIISummer16NanoAODv7__PUSummer16v3Fast_Nano02Apr2020_GridpackScan_102X_mcRun2_asymptotic_v8-v1__10000__EEF1A2CA-4794-EC45-BE9A-4CD629315C1C.root"
      command = 'scripts/split_gluino.py -i "'+dataset_glob+'" -o '+args.target_dir+' -n '+nlsp_mass+' -l '+lsp_mass+' -e '+entrylist_dir
      commands.append(command)
    # Used for debugging
    #break

  # Write commands to command file
  with open(args.out_cmd_filename,'w') as cmd_file:
    cmd_file.write('#!/bin/env python\n')
    for command in commands:
      cmd_file.write('print("""'+command+'""")\n')
    print('Wrote '+args.out_cmd_filename)
  os.chmod(args.out_cmd_filename, 0o755)

  # Make target directory
  if not os.path.exists(args.target_dir):
    os.makedirs(args.target_dir)

  print("To generate job json and submit jobs do: ")
  print('convert_cl_to_jobs_info.py '+args.out_cmd_filename+' '+os.path.splitext(args.out_cmd_filename)[0]+'.json')
  print('auto_submit_jobs.py '+os.path.splitext(args.out_cmd_filename)[0]+'.json -c jobscript_check.py -n cms1')
