from bs4 import BeautifulSoup
import re
from typing import List
import os

def convert_file(input: str, output: str) -> None:
    inbase = os.path.split(input)[0]
    if inbase == '':
        inbase = '.'
    with open(input) as f:
        soup = BeautifulSoup(f, 'html.parser')
        links = soup.find_all('link')
        for link in links:
            ref = os.sep.join((inbase, link['href']))
            print(f'Inbase: {inbase}, Link: {link}, Ref: {ref}')
            tag = soup.new_tag('style')
            with open(ref) as r:
                contents = r.read()
                contents = " ".join(contents.split())
                tag.string = contents
            link.replace_with(tag)
        scripts = soup.find_all('script')
        for script in scripts:
            try:
                src = os.sep.join((inbase, script['src']))
                tag = soup.new_tag('script')
                with open(src) as s:
                    contents = s.read()
                    contents = re.sub(' +', ' ', contents)
                    contents = re.sub('\n+', '\n', contents)
                    tag.string = contents
                script.replace_with(tag)
            except KeyError:
                pass
    # We need to generate the output directory tree if it doesn't exist
    outbase = os.path.split(output)[0]
    os.makedirs(outbase, exist_ok=True)
    with open(output, 'w') as f:
        f.write(str(soup))

def traverse_directory(dirname: str) -> List[str]:
    targets = []
    for root, dirs, files in os.walk(dirname):
        path = root.split(os.sep)
        relative = os.sep.join(path[1:])
        for file in files:
            if file.endswith('.htm'):
                targets.append(os.path.join(relative, file))
    return targets

def convert_directory(indir: str, outdir: str) -> None:
    targets = traverse_directory(indir)
    for target in targets:
        outname = os.path.join(outdir, target)
        print(f'Converting {target} to {outname} ...')
        convert_file(target, outname)
