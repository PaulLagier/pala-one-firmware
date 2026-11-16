"""Validate catalog gates and deterministic generated language tables."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'scripts/generate_locales.py'
CATALOGS = ROOT / 'Pala_One_2_1/src/lang/catalogs'

class GeneratorTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.folder = Path(self.temporary.name)
        self.catalogs = self.folder / 'catalogs'
        shutil.copytree(CATALOGS, self.catalogs)
        self.output = self.folder / 'out'
        self.output.mkdir()
        (self.output / 'runtime_strings.h').write_text('previous valid output')

    def generate(self):
        return subprocess.run([sys.executable, '-B', str(SCRIPT),
            '--catalogs', str(self.catalogs), '--out', str(self.output),
            '--interface-source', str(ROOT / 'Pala_One_2_1/src'), '--require-all'],
            capture_output=True, text=True)

    def edit(self, key, value):
        path = self.catalogs / 'de.json'
        data = json.loads(path.read_text())
        if value is None:
            del data[key]
        else:
            data[key] = value
        path.write_text(json.dumps(data, ensure_ascii=False))

    def rejected(self, reason):
        result = self.generate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(reason, result.stdout + result.stderr)
        self.assertEqual((self.output / 'runtime_strings.h').read_text(), 'previous valid output')

    def test_all_languages_and_output_are_deterministic(self):
        result = self.generate()
        self.assertEqual(result.returncode, 0, result.stderr)
        manifest = json.loads((self.output / 'locale-manifest.json').read_text())
        self.assertEqual(manifest['keyCount'], 521)
        self.assertEqual(len(manifest['implementedCatalogs']), 11)
        self.assertLess(manifest['interfacePoolCount'], manifest['poolsPerCatalog'])
        self.assertLessEqual(manifest['largestAllocation'], 2048)
        before = {p.name: p.read_bytes() for p in self.output.iterdir()}
        self.assertEqual(self.generate().returncode, 0)
        self.assertEqual(before, {p.name: p.read_bytes() for p in self.output.iterdir()})

    def test_missing_language(self):
        (self.catalogs / 'uk.json').unlink()
        self.rejected('Incomplete eleven-language release')

    def test_missing_key(self):
        self.edit('D_BLE_PIN', None)
        self.rejected('missing/extra keys')

    def test_empty_nul_and_non_nfc(self):
        for value, reason in [('', 'invalid text'), ('bad\0text', 'invalid text'), ('e\u0301', 'not NFC')]:
            self.edit('D_BLE_PIN', value)
            self.rejected(reason)

    def test_native_printf_placeholders(self):
        self.edit('D_READER_MENU_PAGE_OF_FMT', 'Seite %s von %d  (%d%%)')
        self.rejected('printf placeholders differ')

    def test_javascript_numbered_placeholders(self):
        self.edit('D_WEB_READ_JS_MATCH_FMT', 'Treffer %1 von %2 (Byte %9)')
        self.rejected('JavaScript placeholders differ')

    def test_html_markup(self):
        self.edit('D_WEB_UPLOAD_BOOK_DESC', 'Bücher nach <i>/books</i> senden.')
        self.rejected('HTML markup differs')

    def test_inline_confirmation_injection(self):
        self.edit('D_WEB_CONFIRM_DELETE_FILE', "Don't delete this book?")
        self.rejected('unsafe inline confirmation')

    def test_single_string_allocation_bound(self):
        self.edit('D_BLE_PIN', 'x' * 2049)
        self.rejected('Single catalog string exceeds 2KB')

if __name__ == '__main__':
    unittest.main()
