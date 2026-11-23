"""Exercise packaged build identity through the actual packaging CLI."""
import json
import subprocess
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / 'Tools/Package.py'


class PackageTest(unittest.TestCase):
    def setUp(self):
        temporary_root = (ROOT / '.tmp').resolve()
        temporary_root.mkdir(exist_ok=True)
        directory = tempfile.TemporaryDirectory(prefix='package-', dir=temporary_root)
        self.player = Path(directory.name).resolve()
        assert self.player.is_relative_to(temporary_root)
        self.addCleanup(directory.cleanup)
        for relative in (
            'StillAwake.exe',
            'StillAwake/Binaries/Win64/StillAwake.exe',
            'StillAwake/Content/Paks/StillAwake-Windows.pak',
            'StillAwake/Content/Paks/StillAwake-Windows.utoc',
            'StillAwake/Content/Paks/StillAwake-Windows.ucas',
            'StillAwake/Content/Data/content.json',
        ):
            path = self.player / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'fixture')
        self.stamp = self.player / 'StillAwake/build-info.json'
        self.identity = {
            'version': '0.3.0',
            'buildNumber': '12',
            'buildVersion': '0.3.0+build.12',
            'runId': 'fixture-12',
            'configuration': 'Development',
        }
        self.write_stamp(self.identity)

    def write_stamp(self, identity):
        self.stamp.write_text(json.dumps(identity), encoding='utf-8')

    def run_verification(self):
        return subprocess.run(
            [sys.executable, str(TOOL), 'verify-player', str(self.player)],
            capture_output=True, text=True, encoding='utf-8',
        )

    def assert_accepts_stamp(self):
        result = self.run_verification()
        self.assertEqual(result.returncode, 0, result.stderr)

    def assert_rejects_stamp(self, message):
        result = self.run_verification()
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn(message, result.stderr)

    def test_accepts_valid_builds(self):
        for configuration in ('Development', 'Shipping'):
            with self.subTest(configuration=configuration):
                self.identity['configuration'] = configuration
                self.write_stamp(self.identity)
                self.assert_accepts_stamp()

    def test_preserves_existing_semver_metadata(self):
        self.identity['version'] = '0.3.0-rc.1+steam'
        self.identity['buildVersion'] = '0.3.0-rc.1+steam.build.12'
        self.write_stamp(self.identity)
        self.assert_accepts_stamp()

    def test_rejects_missing_stamp(self):
        self.stamp.unlink()
        self.assert_rejects_stamp('Missing build-info.json')

    def test_rejects_malformed_stamp(self):
        for text in ('{', '[]'):
            with self.subTest(text=text):
                self.stamp.write_text(text, encoding='utf-8')
                self.assert_rejects_stamp('build-info.json')

    def test_rejects_incomplete_stamp(self):
        for field in self.identity:
            with self.subTest(missing=field):
                identity = dict(self.identity)
                identity.pop(field)
                self.write_stamp(identity)
                self.assert_rejects_stamp('Build stamp requires nonempty strings')
            for value in (None, '', ' ', 12):
                with self.subTest(field=field, value=value):
                    identity = dict(self.identity)
                    identity[field] = value
                    self.write_stamp(identity)
                    self.assert_rejects_stamp('Build stamp requires nonempty strings')

    def test_rejects_invalid_build_number(self):
        for number in ('0', '01', '-1', '1.0', '\u0661', '12\n'):
            with self.subTest(number=number):
                self.identity['buildNumber'] = number
                self.write_stamp(self.identity)
                self.assert_rejects_stamp('Invalid build number')

    def test_rejects_mismatched_build_version(self):
        self.identity['buildVersion'] = '0.3.0+build.11'
        self.write_stamp(self.identity)
        self.assert_rejects_stamp('Build version differs from version and build number')

    def test_rejects_unknown_configuration(self):
        self.identity['configuration'] = 'Debug'
        self.write_stamp(self.identity)
        self.assert_rejects_stamp('Invalid build configuration')

    def test_includes_stamp_in_player_archive(self):
        output = self.player / 'fixture.zip'
        result = subprocess.run(
            [sys.executable, str(TOOL), 'player', str(self.player), str(output)],
            capture_output=True, text=True, encoding='utf-8',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        with zipfile.ZipFile(output) as archive:
            self.assertEqual(json.loads(archive.read('StillAwake/build-info.json')), self.identity)


if __name__ == '__main__':
    unittest.main()
