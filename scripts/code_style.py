#!/usr/bin/env python

import os
from pathlib import Path

opts = [
'--suffix=none',
'--style=allman',
'--attach-inlines',
'--indent-switches',
'--indent-preproc-define',
'--indent=spaces=4',
'--min-conditional-indent=2',
'--break-blocks',
'--pad-oper',
'--pad-header',
'--unpad-paren',
'--align-pointer=type',
'--align-reference=type',
'--indent-modifiers',
'--attach-classes',
'--break-after-logical',
'--keep-one-line-statements',
'--indent-after-parens',
'--add-braces',
'--max-code-length=120',
'--max-continuation-indent=120',
]

script_path = Path(os.path.realpath(__file__))
src_path = script_path.parents[1] / 'src'
srcs = str(src_path)
cmd = 'astyle ' + ' '.join(opts) + ' --recursive ' + srcs + '/*.c,*.h'

os.system(cmd)
print(srcs, 'style fixed')