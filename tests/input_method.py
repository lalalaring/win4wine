#!/usr/bin/env python3
"""The session gives fcitx5 the input method of the XP media's language.

  tests/input_method.py

It checks session/input_method.py without fcitx5: a missing profile is written
with the language's input method, an existing profile keeps every entry it has
and only gets the missing one appended, a profile that already lists it is left
alone, and a language without an input method (English XP) writes nothing.
"""
from pathlib import Path
import sys
import tempfile
import unittest
root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'session'))
import input_method  # noqa: E402

# A profile as fcitx5 writes its own first one, with entries the user added.
PROFILE = '''[Groups/0]
Name=Default
Default Layout=us
DefaultIM=keyboard-us

[Groups/0/Items/0]
Name=keyboard-us
Layout=

[Groups/0/Items/1]
Name=anthy
Layout=

[GroupOrder]
0=Default
'''


class InputMethod(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix='input-method-')
        self.profile = Path(self.directory.name) / 'fcitx5/profile'

    def tearDown(self):
        self.directory.cleanup()

    def test_missing_profile(self):
        # The keyboard layout comes from the X server when setxkbmap can say
        # it; either way the language's input method is in the new profile.
        self.assertEqual(input_method.state('zh', self.profile, write=False), 'created')
        self.assertEqual(input_method.ensure('zh', self.profile), 'created')
        text = self.profile.read_text()
        self.assertIn('Name=pinyin', text)
        self.assertIn('[Groups/0/Items/1]', text)
        self.assertIn('DefaultIM=keyboard-us', text)
        self.assertIn('[GroupOrder]', text)

    def test_japanese_language(self):
        self.assertEqual(input_method.INPUT_METHODS['ja'][0], 'mozc')
        input_method.ensure('ja', self.profile)
        self.assertIn('Name=mozc', self.profile.read_text())

    def test_existing_profile_keeps_its_entries(self):
        self.profile.parent.mkdir(parents=True)
        self.profile.write_text(PROFILE)
        self.assertEqual(input_method.ensure('zh', self.profile), 'added')
        text = self.profile.read_text()
        # The user's own input method is kept and the new one is numbered after it.
        self.assertIn('Name=anthy', text)
        self.assertIn('[Groups/0/Items/2]', text)
        self.assertIn('Name=pinyin', text)
        self.assertIn('[GroupOrder]', text)

    def test_second_call_changes_nothing(self):
        self.profile.parent.mkdir(parents=True)
        self.profile.write_text(PROFILE)
        input_method.ensure('zh', self.profile)
        once = self.profile.read_text()
        self.assertIsNone(input_method.ensure('zh', self.profile))
        self.assertEqual(self.profile.read_text(), once)

    def test_profile_without_default_group(self):
        self.profile.parent.mkdir(parents=True)
        self.profile.write_text('[Groups/1]\nName=Other\n')
        # Left alone: the user adds the input method with fcitx5-configtool.
        self.assertEqual(input_method.ensure('zh', self.profile), 'unsupported')
        self.assertEqual(self.profile.read_text(), '[Groups/1]\nName=Other\n')

    def test_english_writes_nothing(self):
        self.assertIsNone(input_method.state('en', self.profile, write=False))
        self.assertFalse(self.profile.exists())

    def test_names_and_numbers(self):
        self.assertEqual(input_method.names(PROFILE), ['keyboard-us', 'anthy'])
        self.assertEqual(input_method.item_number(PROFILE), 2)
        self.assertEqual(input_method.item_number('[Groups/0]\nName=Default\n'), 0)
        added = input_method.add(PROFILE, 'pinyin', 2)
        self.assertTrue(added.endswith('[Groups/0/Items/2]\nName=pinyin\nLayout=\n'))


if __name__ == '__main__':
    unittest.main()
