#!/usr/bin/env python3
"""Copy reviewed SDK/corpus bytes and pin every file; no network or dependencies."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
ROOT = Path(__file__).resolve().parent.parent


def files(directory):
    return sorted(p for p in directory.rglob('*') if p.is_file() and '__pycache__' not in p.parts and p.suffix != '.pyc')


def hashes(directory):
    return {p.relative_to(directory).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in files(directory)}


def main():
    mode = sys.argv[1]
    if mode not in ('sdk', 'suite', 'check'):
        sys.exit('usage: sync.py sdk|suite [SDK_DIR] | check')
    if mode == 'check':
        for name, directory in [('sdk', ROOT/'vendor/lmcode'), ('suite', ROOT/'sample-suite')]:
            lock = json.loads((ROOT/(name+'.lock')).read_text())
            if hashes(directory) != lock['sha256']:
                sys.exit(name + ' drift: sync reviewed upstream bytes, do not edit vendored files')
        print('PASS SDK and suite locks')
        return
    source = Path(sys.argv[2]).resolve() if len(sys.argv)>2 else ROOT.parent/'lmcode-cpp'
    commit = subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()
    paths = ['include','stdlib','std','LICENSE'] if mode=='sdk' else ['sample-suite']
    dirty = subprocess.check_output(['git','-C',str(source),'status','--porcelain','--',*paths],text=True)
    if dirty:
        sys.exit('commit the source SDK/corpus before syncing')
    destination = ROOT/'vendor/lmcode' if mode=='sdk' else ROOT/'sample-suite'
    if destination.exists(): shutil.rmtree(destination)
    destination.mkdir(parents=True)
    if mode=='sdk':
        selected = [source/'include/dslvm.hpp',source/'std/lmcode-std.schema.json',source/'LICENSE',*sorted((source/'stdlib').glob('*.hpp'))]
        for path in selected:
            target=destination/path.relative_to(source);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,target)
    else:
        for path in files(source/'sample-suite'):
            target=destination/path.relative_to(source/'sample-suite');target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,target)
    (ROOT/(mode+'.lock')).write_text(json.dumps({'repo':'lmctl-ai/lmcode-cpp','commit':commit,'sha256':hashes(destination)},indent=2)+'\n',newline='\n')
    print(f'Pinned {mode} at {commit}')

if __name__=='__main__':
    main()
