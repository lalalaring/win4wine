#!/usr/bin/env python3
"""Give fcitx5 the input method that the session's language needs.

Debian's fcitx5 does not add an input method by itself: until one is added,
Ctrl+Space does nothing and Wine's and Linux's text fields take no Japanese or
Chinese. The session calls ensure() before it starts fcitx5, so the XP shell
has the input method its language uses from the first login.

fcitx5 keeps its input methods in ~/.config/fcitx5/profile. An existing profile
keeps every entry the user has: a missing input method is appended to the
Default group, and the file is only written when something is missing. A
profile that does not exist yet is written whole, with the keyboard layout the
running X server reports (setxkbmap), or the language's usual text layout.

  input_method.py LANGUAGE [--profile PATH]   reports what the session would
                                              do, without writing anything
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys

# The input method (fcitx5's addon name) and the keyboard layout of each
# language of the XP media. Japanese XP's IME is Mozc, Simplified Chinese XP's
# is Pinyin (fcitx5-chinese-addons); both are switched with Ctrl+Space.
INPUT_METHODS = {'ja': ('mozc', 'jp'), 'zh': ('pinyin', 'us')}
GROUP = 'Groups/0'


def profile_path():
    """The fcitx5 profile of the session's user."""
    base = Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config')
    return base / 'fcitx5' / 'profile'


def keyboard_layout():
    """The first layout of the running X server, or None.

    fcitx5's own first profile takes the layout from its session; a profile
    written here takes it from the X server, which may be in another session."""
    try:
        result = subprocess.run(['setxkbmap', '-query'], capture_output=True, timeout=5)
    except (OSError, subprocess.SubprocessError):
        return None
    if result.returncode:
        return None
    found = re.search(r'^\s*layout:\s*(\S+)', result.stdout.decode('utf-8', 'replace'), re.MULTILINE)
    return found.group(1).split(',')[0] if found else None


def names(text):
    """The input methods a profile lists, in the order it lists them.

    Only the entries of an Items section count: a group has a name of its own
    ([Groups/0] Name=Default), which is not an input method."""
    found, item = [], False
    for line in text.splitlines():
        line = line.strip()
        if line.startswith('['):
            item = bool(re.match(re.escape('[Groups/') + r'\d+/Items/\d+\]', line))
        elif item:
            match = re.match(r'Name\s*=\s*(\S+)', line)
            if match:
                found.append(match.group(1))
    return found


def item_number(text):
    """The number to give the next input method of the Default group."""
    numbers = [int(n) for n in re.findall(re.escape(f'[{GROUP}/Items/') + r'(\d+)\]', text)]
    return max(numbers, default=-1) + 1


def default_profile(input_method, layout):
    """The profile fcitx5 writes for itself, with INPUT_METHOD added."""
    return (f'[{GROUP}]\n'
            'Name=Default\n'
            f'Default Layout={layout}\n'
            'DefaultIM=keyboard-us\n'
            '\n'
            f'[{GROUP}/Items/0]\n'
            'Name=keyboard-us\n'
            'Layout=\n'
            '\n'
            f'[{GROUP}/Items/1]\n'
            f'Name={input_method}\n'
            'Layout=\n'
            '\n'
            '[GroupOrder]\n'
            '0=Default\n')


def add(text, input_method, number):
    """TEXT with INPUT_METHOD appended to the Default group."""
    return (text.rstrip('\n') + '\n'
            '\n'
            f'[{GROUP}/Items/{number}]\n'
            f'Name={input_method}\n'
            'Layout=\n')


def state(language, profile=None, write=True):
    """Put LANGUAGE's input method into the fcitx5 profile when it is missing.

    With WRITE, the profile is written; without it, nothing changes and the
    answer is what a session would do. The answer is 'created' (a new profile),
    'added' (appended to the existing one), 'unsupported' (a profile whose
    Default group cannot be found: the user adds the input method with
    fcitx5-configtool) or None when there is nothing to do (a language without
    an input method, or one that is already there)."""
    if language not in INPUT_METHODS:
        return None
    input_method, layout = INPUT_METHODS[language]
    profile = Path(profile) if profile else profile_path()
    try:
        text = profile.read_text()
    except OSError:
        text = None
    if text is not None and input_method in names(text):
        return None
    if text is None:
        content, answer = default_profile(input_method, keyboard_layout() or layout), 'created'
    elif f'[{GROUP}]' not in text:
        return 'unsupported'
    else:
        content, answer = add(text, input_method, item_number(text)), 'added'
    if write:
        profile.parent.mkdir(parents=True, exist_ok=True)
        temporary = profile.with_name(profile.name + '.win4wine')
        temporary.write_text(content)
        temporary.replace(profile)
    return answer


def ensure(language, profile=None):
    """state() as the session calls it: the profile is written when it has to be."""
    return state(language, profile)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('language', choices=sorted(INPUT_METHODS))
    parser.add_argument('--profile', type=Path)
    args = parser.parse_args()
    print(state(args.language, args.profile, write=False) or 'present')
