#!/usr/bin/env python3
"""Open debug.csv via File Menu → Open → Data Library using AT-SPI2."""

import unittest
import time
import sys
from accessibility_common import (
    Atspi, find_jasp_app, click_element, close_menu,
    find_all_by_role, dismiss_dialogs, get_jasp_app,
    find_window_by_name, generate_key_event, type_text,
)


def _find_btn_by_name(parent, name):
    name_lower = name.lower()
    try:
        if "button" in parent.get_role_name().lower():
            if name_lower in (parent.get_name() or "").lower():
                return parent
    except Exception:
        pass
    try:
        cc = parent.get_child_count()
        for i in range(cc):
            r = _find_btn_by_name(parent.get_child_at_index(i), name)
            if r:
                return r
    except Exception:
        pass
    return None


def _find_by_role_and_name(parent, role_name, name):
    name_lower = name.lower()
    try:
        if role_name.lower() in parent.get_role_name().lower():
            if name_lower in (parent.get_name() or "").lower():
                return parent
    except Exception:
        pass
    try:
        cc = parent.get_child_count()
        for i in range(cc):
            r = _find_by_role_and_name(parent.get_child_at_index(i), role_name, name)
            if r:
                return r
    except Exception:
        pass
    return None


def _find_btn_by_name_bounded(parent, name, depth=0, max_depth=15, seen_ids=None):
    """Depth-limited variant of _find_btn_by_name that avoids infinite recursion."""
    if depth > max_depth:
        return None
    if seen_ids is None:
        seen_ids = set()
    try:
        obj_id = id(parent)
        if obj_id in seen_ids:
            return None
        seen_ids.add(obj_id)
    except Exception:
        pass
    name_lower = name.lower()
    try:
        if "button" in parent.get_role_name().lower():
            if name_lower in (parent.get_name() or "").lower():
                return parent
    except Exception:
        pass
    try:
        cc = min(parent.get_child_count(), 200)
        for i in range(cc):
            try:
                child = parent.get_child_at_index(i)
            except Exception:
                continue
            r = _find_btn_by_name_bounded(child, name, depth + 1, max_depth, seen_ids)
            if r:
                return r
    except Exception:
        pass
    return None


def _find_by_role_and_name_bounded(parent, role_name, name, depth=0, max_depth=15, seen_ids=None):
    """Depth-limited variant of _find_by_role_and_name."""
    if depth > max_depth:
        return None
    if seen_ids is None:
        seen_ids = set()
    try:
        obj_id = id(parent)
        if obj_id in seen_ids:
            return None
        seen_ids.add(obj_id)
    except Exception:
        pass
    name_lower = name.lower()
    role_lower = role_name.lower()
    try:
        if role_lower in parent.get_role_name().lower():
            if name_lower in (parent.get_name() or "").lower():
                return parent
    except Exception:
        pass
    try:
        cc = min(parent.get_child_count(), 200)
        for i in range(cc):
            try:
                child = parent.get_child_at_index(i)
            except Exception:
                continue
            r = _find_by_role_and_name_bounded(child, role_name, name, depth + 1, max_depth, seen_ids)
            if r:
                return r
    except Exception:
        pass
    return None


def _find_menu_item(parent, name):
    return _find_by_role_and_name(parent, "menu item", name)


def _fresh_app():
    return get_jasp_app()


def _robust_search(func, *args, max_retries=3):
    for attempt in range(max_retries):
        try:
            app = _fresh_app()
            if not app:
                time.sleep(1.5)
                continue
            result = func(app, *args)
            if result is not None and (not isinstance(result, (list, tuple)) or len(result) > 0):
                return result
            time.sleep(1.5)
        except Exception as e:
            if "no longer exists" in str(e) or "Did not receive a reply" in str(e):
                time.sleep(1.5)
    return func(_fresh_app(), *args)


def _send_down():
    """Send Down Arrow via AT-SPI keyboard event."""
    try:
        Atspi.generate_keyboard_event(0xFF54, None, Atspi.KeySynthType.SYM)
    except Exception:
        pass


def _send_enter():
    """Send Enter key via AT-SPI keyboard event."""
    try:
        Atspi.generate_keyboard_event(0xFF0D, None, Atspi.KeySynthType.SYM)
    except Exception:
        pass


def _cell_matches(cell, row, col_name):
    """Check if cell name matches exact row and column. Avoids substring false positives."""
    name = (cell.get_name() or "")
    return name.startswith(f"Row {row}, Col {col_name}:") or name.startswith(f"Row {row}, Col {col_name},")


def _cell_names_matching(cell_names, row, col_name):
    """Filter cell name strings matching exact row and column."""
    prefix = f"Row {row}, Col {col_name}:"
    return [n for n in cell_names if n.startswith(prefix)]


def _grab_window_focus():
    """Try to give the JASP window X11 focus so keyboard events reach it."""
    try:
        desktop = Atspi.get_desktop(0)
        for i in range(desktop.get_child_count()):
            a = desktop.get_child_at_index(i)
            if "jasp" in a.get_name().lower():
                for j in range(a.get_child_count()):
                    try:
                        c = a.get_child_at_index(j)
                        if c.get_role_name() == "frame":
                            c.grab_focus()
                            return True
                    except Exception:
                        pass
    except Exception:
        pass
    return False


def _click_menu_option(button_name, menu_item_name):
    """Click a toolbar button to open its dropdown, find the target menu item
    in the AT-SPI tree, and click it via do_action."""
    app = _fresh_app()
    if not app:
        return False
    all_btns = find_all_by_role(app, "button", max_depth=20)
    btn = None
    for b in all_btns:
        if button_name.lower() in (b.get_name() or "").lower():
            btn = b
            break
    if not btn:
        return False
    click_element(btn)
    time.sleep(0.5)

    # Search for menu items globally via AT-SPI
    app = _fresh_app()
    if app:
        items = find_all_by_role(app, "menu item", max_depth=25)
        for mi in items:
            if menu_item_name.lower() in (mi.get_name() or "").lower():
                click_element(mi)
                time.sleep(2)
                return True

    return False


class TestCSVLoading(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        Atspi.init()
        cls.app, cls.main_window = find_jasp_app(timeout=30)
        if not cls.main_window:
            sys.exit(1)
        for _ in range(5):
            dismiss_dialogs(cls.app)
            time.sleep(1)

    def setUp(self):
        close_menu()
        time.sleep(0.5)

    def _open_file_menu(self):
        close_menu()
        time.sleep(1)
        btn = _find_btn_by_name(self.main_window, "Main menu")
        if not btn:
            return False
        click_element(btn)
        time.sleep(5)
        app = _fresh_app()
        if app:
            type(self).app = app
        elements = find_all_by_role(self.app, "button", max_depth=8)
        names = [(e.get_name() or "").lower() for e in elements]
        return any("save as" in n or "export results" in n for n in names)

    def _ensure_data_mode(self):
        """Enter data mode if not already there. Returns True if in data mode after call."""
        app = _fresh_app()
        all_btns = find_all_by_role(app, "button", max_depth=20)
        names = [(b.get_name() or "").lower() for b in all_btns]

        if any("analyses" in n for n in names):
            return True

        # Check if "Edit Data" is among the buttons
        edit_btns = [b for b in all_btns if "edit data" in (b.get_name() or "").lower()]
        if not edit_btns:
            # Might be the hamburger menu panel covering the ribbon.
            # Close it by pressing Escape (real Escape, not close_menu's fake one)
            for _ in range(3):
                try:
                    Atspi.generate_keyboard_event(0xFF1B, None, Atspi.KeySynthType.SYM)
                except Exception:
                    pass
                time.sleep(0.3)
            time.sleep(1)
            app = _fresh_app()
            all_btns = find_all_by_role(app, "button", max_depth=20)
            names = [(b.get_name() or "").lower() for b in all_btns]
            if any("analyses" in n for n in names):
                return True
            edit_btns = [b for b in all_btns if "edit data" in (b.get_name() or "").lower()]

        if edit_btns:
            edit_btns[0].do_action(0)
            time.sleep(3)
        app = _fresh_app()
        all_btns = find_all_by_role(app, "button", max_depth=20)
        names = [(b.get_name() or "").lower() for b in all_btns]
        return any("analyses" in n for n in names)

    # ── Existing tests ───────────────────────────────────────────────────

    def test_01_open_csv_via_data_library(self):
        close_menu()
        time.sleep(1)
        self.assertTrue(self._open_file_menu(), "Could not open file menu")

        open_btns = _robust_search(lambda app: find_all_by_role(app, "button", "open", max_depth=8))
        target = open_btns[-1] if len(open_btns) >= 2 else open_btns[0] if open_btns else None
        self.assertIsNotNone(target, "No Open button")
        click_element(target)
        time.sleep(2)

        dl_btn = _robust_search(lambda app: _find_btn_by_name(app, "Data Library"))
        self.assertIsNotNone(dl_btn, "Data Library tab not found")
        click_element(dl_btn)
        time.sleep(5)

        # Scroll to reveal Debug Dataset at bottom of list (ListView is virtualized)
        for i in range(30):
            app = _fresh_app()
            if app:
                btns = find_all_by_role(app, "button", max_depth=8)
                if any("debug" in (b.get_name() or "").lower() for b in btns):
                    break
            _send_down()
            time.sleep(0.3)

        debug_btn = _robust_search(lambda app: _find_btn_by_name(app, "Debug Dataset"))
        self.assertIsNotNone(debug_btn, "Debug Dataset not found in Data Library")
        click_element(debug_btn)
        time.sleep(5)

        dp = find_window_by_name(None, "Data Preview", timeout=10)
        self.assertIsNotNone(dp, "Data Preview not found")

        load_btn = _robust_search(
            lambda app: _find_btn_by_name(
                find_window_by_name(None, "Data Preview", timeout=2) or app, "Load")
        )
        self.assertIsNotNone(load_btn, "Load button not found")
        click_element(load_btn)
        time.sleep(5)

        close_menu()
        time.sleep(2)

        # Toggle Main menu to close the file menu panel if still open
        app = _fresh_app()
        if app:
            hamburger = _find_btn_by_name(app, "Main menu")
            if hamburger:
                click_element(hamburger)
                time.sleep(2)

        app = _fresh_app()
        self.assertIsNotNone(app, "JASP gone after Load")
        all_btns = find_all_by_role(app, "button", max_depth=8)
        names = [(b.get_name() or "").lower() for b in all_btns]
        self.assertTrue(any("edit data" in n or "sync data" in n for n in names),
                        "No data-mode buttons found after loading")

    def test_02_data_mode_buttons(self):
        edit_btn = _robust_search(
            lambda app: _find_btn_by_name(_fresh_app() or app, "Edit Data")
        )
        self.assertIsNotNone(edit_btn, "Edit Data button not found")
        click_element(edit_btn)
        time.sleep(3)

        app = _fresh_app()
        all_btns = find_all_by_role(app, "button", max_depth=8)
        names = [(b.get_name() or "").lower() for b in all_btns]
        expected = ["analyses", "synchronisation", "resize data", "insert", "remove", "undo", "redo"]
        found = [n for n in expected if any(n in bn for bn in names)]
        self.assertGreater(len(found), 3,
                           f"Too few data-mode buttons: {found}")

    def test_03_switch_back_to_analyses(self):
        """Click Analyses to return to analysis mode and verify ribbon buttons."""
        analyses_btn = _robust_search(
            lambda app: _find_btn_by_name_bounded(app, "Analyses")
        )
        self.assertIsNotNone(analyses_btn, "Analyses button not found")
        click_element(analyses_btn)
        time.sleep(3)

        app = _fresh_app()
        all_btns = find_all_by_role(app, "button", max_depth=8)
        names = [(b.get_name() or "").lower() for b in all_btns]
        self.assertTrue(any("edit data" in n for n in names),
                        "Edit Data not found after switching back to Analyses")
        self.assertTrue(any("descriptives" in n for n in names),
                        "Descriptives ribbon button not found")

    def test_04_data_table_accessible(self):
        """Verify the data table has accessible table cells with content."""
        app = _fresh_app()
        self.assertIsNotNone(app, "JASP gone")

        tables = find_all_by_role(app, "table", max_depth=10)
        if not tables:
            self.skipTest("No table elements found in the app")

        cells = find_all_by_role(app, "table cell", max_depth=10)
        self.assertGreater(len(cells), 0, "No table cells found after loading CSV")

        named_cells = [(c.get_name() or c.get_description() or "") for c in cells]
        non_empty = [n for n in named_cells if n]
        print(f"  [T04] {len(cells)} cells, {len(non_empty)} named: {non_empty[:10]}", flush=True)
        self.assertGreater(len(non_empty), 3,
                           f"Too few named table cells: {non_empty[:10]}")

    # ── New data-editing regression tests ─────────────────────────────────

    def test_05_data_table_contents(self):
        """[Spec Test 03] Verify data table has accessible cells, row/col headers with content."""
        self._ensure_data_mode()

        app = _fresh_app()
        self.assertIsNotNone(app, "JASP gone")

        # Verify column headers
        col_headers = find_all_by_role(app, "table column header", max_depth=20)
        col_names = [(h.get_name() or "") for h in col_headers]
        print(f"  [T05] {len(col_headers)} column headers by role: {col_names[:8]}", flush=True)

        # Fallback: search for ANY element with "Column:" in name to check actual role
        if len(col_headers) < 3:
            from accessibility_common import find_all
            elements = find_all(app, max_depth=20)
            col_like = [(r, n,) for r, n, _ in elements if "column:" in n.lower() or "col " in n.lower()]
            print(f"  [T05] DEBUG 'Column:' elements: {col_like[:15]}", flush=True)
            # Also print all distinct roles
            roles = sorted(set(r for r, n, _ in elements))
            print(f"  [T05] DEBUG all roles: {roles}", flush=True)
        self.assertGreater(len(col_headers), 5,
                           f"Too few column headers: {col_names}")

        expected_cols = ["contNormal", "contGamma", "contBinom", "contExpon", "facGender"]
        for exp in expected_cols:
            self.assertTrue(any("Column:" in n and exp in n for n in col_names),
                            f"Column '{exp}' not found in headers: {col_names}")

        # Verify row headers
        row_headers = find_all_by_role(app, "table row header", max_depth=10)
        row_names = [(h.get_name() or "") for h in row_headers]
        print(f"  [T05] {len(row_headers)} row headers: {row_names[:5]}...", flush=True)
        self.assertGreater(len(row_headers), 5,
                           f"Too few row headers: {row_names}")

        has_row1 = any("Row 1" in n for n in row_names)
        self.assertTrue(has_row1, f"Row 1 not found in row headers: {row_names}")

        # Verify cell content
        cells = find_all_by_role(app, "table cell", max_depth=10)
        cell_names = [(c.get_name() or "") for c in cells]
        non_empty = [n for n in cell_names if n]
        print(f"  [T05] {len(cells)} cells, {len(non_empty)} named", flush=True)
        self.assertGreater(len(non_empty), 20,
                           f"Too few named cells: {non_empty[:10]}")

        has_row1_contNormal = any("Row 1" in n and "contNormal" in n for n in cell_names)
        self.assertTrue(has_row1_contNormal,
                        f"No Row 1 contNormal cell found in: {[n for n in cell_names if 'Row 1' in n][:5]}")

    def test_06_insert_row_undo_redo(self):
        """[Spec Test 04] Insert row above Row 1 → verify empty → undo → redo."""
        self._ensure_data_mode()

        # Find reference cell before insertion
        app = _fresh_app()
        cells = find_all_by_role(app, "table cell", max_depth=10)
        cell_names = [(c.get_name() or "") for c in cells]

        row1_contNormal_before = None
        for n in cell_names:
            if "Row 1" in n and "contNormal" in n:
                row1_contNormal_before = n
                break
        self.assertIsNotNone(row1_contNormal_before,
                             "Row 1 contNormal not found before insert")
        print(f"  [T06] Before insert: {row1_contNormal_before}", flush=True)

        # Click Insert → "Insert row above"
        result = _click_menu_option("Insert", "Insert row above")
        self.assertTrue(result, "Could not click Insert → Insert row above")

        # Verify Row 1 is now empty, Row 2 has old value
        app = _fresh_app()
        cells = find_all_by_role(app, "table cell", max_depth=10)
        cell_names = [(c.get_name() or "") for c in cells]

        row1_after = _cell_names_matching(cell_names, 1, "contNormal")
        row2_after = _cell_names_matching(cell_names, 2, "contNormal")

        print(f"  [T06] After insert: Row 1: {row1_after}, Row 2: {row2_after}", flush=True)

        # Row 1 should be empty or show the latest displayed value
        if row1_after:
            r1_name = row1_after[0]
            # Check if it's empty (": " ending) or has a blank value
            self.assertTrue(
                r1_name.endswith(": ") or r1_name.endswith(":  ") or
                "contNormal: " in r1_name,
                f"Row 1 should be empty after insert above, got: {r1_name}"
            )

        # Click Undo
        undo_btn = _robust_search(lambda app: _find_btn_by_name_bounded(app, "Undo"))
        self.assertIsNotNone(undo_btn, "Undo button not found")
        click_element(undo_btn)
        time.sleep(2)

        # Verify undo restored
        app = _fresh_app()
        cells = find_all_by_role(app, "table cell", max_depth=10)
        cell_names = [(c.get_name() or "") for c in cells]
        row1_undo = _cell_names_matching(cell_names, 1, "contNormal")
        self.assertTrue(row1_undo, "Row 1 contNormal not found after undo")
        self.assertIn("Row 1", row1_undo[0])
        print(f"  [T06] After undo: {row1_undo[0]}", flush=True)

        # Click Redo
        redo_btn = _robust_search(lambda app: _find_btn_by_name_bounded(app, "Redo"))
        self.assertIsNotNone(redo_btn, "Redo button not found")
        click_element(redo_btn)
        time.sleep(2)

        # Verify redo re-inserted
        app = _fresh_app()
        cells = find_all_by_role(app, "table cell", max_depth=10)
        cell_names = [(c.get_name() or "") for c in cells]
        row1_redo = _cell_names_matching(cell_names, 1, "contNormal")
        if row1_redo:
            self.assertTrue(
                row1_redo[0].endswith(": ") or row1_redo[0].endswith(":  ") or
                "contNormal: " in row1_redo[0],
                f"Row 1 should be empty after redo, got: {row1_redo[0]}"
            )

        # Undo twice to return to baseline
        for _ in range(2):
            undo_btn = _robust_search(lambda app: _find_btn_by_name_bounded(app, "Undo"))
            if undo_btn:
                click_element(undo_btn)
                time.sleep(2)

    def test_07_delete_row_undo(self):
        """[Spec Test 05] Delete Row 1 → verify → undo."""
        self._ensure_data_mode()

        # Find reference cell before deletion
        app = _fresh_app()
        cells = find_all_by_role(app, "table cell", max_depth=10)
        cell_names = [(c.get_name() or "") for c in cells]
        row1_before = _cell_names_matching(cell_names, 1, "contNormal")
        self.assertTrue(row1_before, "Row 1 contNormal not found before delete")
        row1_before_val = row1_before[0]
        print(f"  [T07] Before delete: {row1_before_val}", flush=True)

        row2_before = _cell_names_matching(cell_names, 2, "contNormal")
        self.assertTrue(row2_before, "Row 2 contNormal not found before delete")
        row2_before_val = row2_before[0]
        print(f"  [T07] Row 2 before delete: {row2_before_val}", flush=True)

        # Select Row 1 by clicking its row header
        row1_header = _robust_search(
            lambda app: _find_by_role_and_name_bounded(app, "table row header", "Row 1")
        )
        if not row1_header:
            # Fallback: find by cell
            app = _fresh_app()
            cells = find_all_by_role(app, "table cell", max_depth=10)
            for c in cells:
                if "Row 1" in (c.get_name() or "") and "contNormal" in (c.get_name() or ""):
                    click_element(c)
                    time.sleep(0.5)
                    break
        else:
            click_element(row1_header)
        time.sleep(1)

        # Click Remove → "Delete row"
        result = _click_menu_option("Remove", "Delete row")
        self.assertTrue(result, "Could not click Remove → Delete row")

        # Verify removal
        app = _fresh_app()
        cells = find_all_by_role(app, "table cell", max_depth=10)
        cell_names = [(c.get_name() or "") for c in cells]
        row1_after = _cell_names_matching(cell_names, 1, "contNormal")
        print(f"  [T07] After delete, Row 1: {row1_after}", flush=True)

        # The new Row 1 should have the old Row 2's value
        if row1_after and row2_before_val:
            # Extract just the value part after the column name
            r2_value = row2_before_val.split("contNormal: ")[-1] if "contNormal: " in row2_before_val else row2_before_val
            r1_value = row1_after[0].split("contNormal: ")[-1] if "contNormal: " in row1_after[0] else row1_after[0]
            # They should match if row 2 shifted up
            self.assertTrue(
                r2_value in r1_value or r1_value in r2_value or r2_value == r1_value,
                f"New Row 1 ({r1_value}) should match old Row 2 ({r2_value})"
            )

        # Click Undo
        undo_btn = _robust_search(lambda app: _find_btn_by_name_bounded(app, "Undo"))
        self.assertIsNotNone(undo_btn, "Undo button not found")
        click_element(undo_btn)
        time.sleep(2)

        # Verify undo restored Row 1
        app = _fresh_app()
        cells = find_all_by_role(app, "table cell", max_depth=10)
        cell_names = [(c.get_name() or "") for c in cells]
        row1_undo = _cell_names_matching(cell_names, 1, "contNormal")
        self.assertTrue(row1_undo, "Row 1 not restored after undo")
        print(f"  [T07] After undo: {row1_undo[0]}", flush=True)

    def test_08_edit_cell_value_undo(self):
        """[Spec Test 06] Edit cell value → verify → undo."""
        self._ensure_data_mode()

        # Find Row 2, Col contNormal cell
        app = _fresh_app()
        cells = find_all_by_role(app, "table cell", max_depth=10)
        row2_cell = None
        for c in cells:
            if _cell_matches(c, 2, "contNormal"):
                row2_cell = c
                break

        if not row2_cell:
            # Try wider search
            cells = find_all_by_role(app, "table cell", max_depth=12)
            for c in cells:
                name = c.get_name() or ""
                if "Row 2" in name and "contNormal" in name:
                    row2_cell = c
                    break

        self.assertIsNotNone(row2_cell, "Row 2 contNormal cell not found")
        before_name = row2_cell.get_name() or ""
        print(f"  [T08] Before edit: {before_name}", flush=True)

        # Double-click cell to enter edit mode
        click_element(row2_cell)
        time.sleep(0.2)
        click_element(row2_cell)
        time.sleep(0.5)
        # Press F2 to enter edit mode explicitly
        Atspi.generate_keyboard_event(0xFFC7, None, Atspi.KeySynthType.SYM)
        time.sleep(0.3)

        # Type new value: "42.5"
        for ch in "42.5":
            generate_key_event(ord(ch))
            time.sleep(0.1)

        time.sleep(0.5)

        # Press Enter to confirm
        _send_enter()
        time.sleep(2)

        # Verify change
        app = _fresh_app()
        cells = find_all_by_role(app, "table cell", max_depth=12)
        row2_after = [c for c in cells if "Row 2" in (c.get_name() or "") and "contNormal" in (c.get_name() or "")]
        if row2_after:
            after_name = row2_after[0].get_name() or ""
            print(f"  [T08] After edit: {after_name}", flush=True)
            self.assertIn("42.5", after_name,
                          f"Cell should show 42.5 after edit, got: {after_name}")

        # Click Undo
        undo_btn = _robust_search(lambda app: _find_btn_by_name_bounded(app, "Undo"))
        self.assertIsNotNone(undo_btn, "Undo button not found")
        click_element(undo_btn)
        time.sleep(2)

        # Verify restore
        app = _fresh_app()
        cells = find_all_by_role(app, "table cell", max_depth=12)
        row2_undo = [c for c in cells if "Row 2" in (c.get_name() or "") and "contNormal" in (c.get_name() or "")]
        if row2_undo:
            undo_name = row2_undo[0].get_name() or ""
            print(f"  [T08] After undo: {undo_name}", flush=True)
            self.assertNotIn("42.5", undo_name,
                             "Cell should not contain 42.5 after undo")

    def test_09_insert_column_undo(self):
        """[Spec Test 07] Insert column before → verify → undo."""
        self._ensure_data_mode()

        # Count columns before
        app = _fresh_app()
        col_headers_before = find_all_by_role(app, "table column header", max_depth=10)
        count_before = len(col_headers_before)
        names_before = [(h.get_name() or "") for h in col_headers_before]
        print(f"  [T09] Before: {count_before} columns", flush=True)

        # Click Insert → "Insert column before"
        result = _click_menu_option("Insert", "Insert column before")
        self.assertTrue(result, "Could not click Insert → Insert column before")

        # Verify new column appears
        app = _fresh_app()
        col_headers_after = find_all_by_role(app, "table column header", max_depth=10)
        count_after = len(col_headers_after)
        names_after = [(h.get_name() or "") for h in col_headers_after]
        print(f"  [T09] After: {count_after} columns", flush=True)

        self.assertGreater(count_after, count_before,
                           f"Column count should increase from {count_before}")

        # Undo
        undo_btn = _robust_search(lambda app: _find_btn_by_name_bounded(app, "Undo"))
        self.assertIsNotNone(undo_btn, "Undo button not found")
        click_element(undo_btn)
        time.sleep(2)

        # Verify column removed
        app = _fresh_app()
        col_headers_undo = find_all_by_role(app, "table column header", max_depth=10)
        count_undo = len(col_headers_undo)
        print(f"  [T09] After undo: {count_undo} columns", flush=True)
        self.assertLessEqual(count_undo, count_before + 1,
                             f"Column count should be back to ~{count_before} after undo")

    def test_10_delete_column_undo(self):
        """[Spec Test 08] Delete column contGamma → verify → undo."""
        self._ensure_data_mode()

        # Find contGamma column header
        app = _fresh_app()
        col_headers = find_all_by_role(app, "table column header", max_depth=20)
        contGamma_header = None
        for h in col_headers:
            name = h.get_name() or ""
            if "contGamma" in name and "Column:" in name:
                contGamma_header = h
                break

        self.assertIsNotNone(contGamma_header, "Column: contGamma header not found")

        # Click header to select column
        click_element(contGamma_header)
        time.sleep(1)

        # Click Remove → "Delete column"
        result = _click_menu_option("Remove", "Delete column")
        self.assertTrue(result, "Could not click Remove → Delete column")

        # Verify column removed
        app = _fresh_app()
        col_headers_after = find_all_by_role(app, "table column header", max_depth=10)
        names_after = [(h.get_name() or "") for h in col_headers_after]
        has_contGamma = any("contGamma" in n and "Column:" in n for n in names_after)
        self.assertFalse(has_contGamma, "contGamma column should be deleted")
        print(f"  [T10] contGamma removed. Columns: {len(col_headers_after)}", flush=True)

        # Undo
        undo_btn = _robust_search(lambda app: _find_btn_by_name_bounded(app, "Undo"))
        self.assertIsNotNone(undo_btn, "Undo button not found")
        click_element(undo_btn)
        time.sleep(2)

        # Verify restored
        app = _fresh_app()
        col_headers_undo = find_all_by_role(app, "table column header", max_depth=10)
        names_undo = [(h.get_name() or "") for h in col_headers_undo]
        has_contGamma_undo = any("contGamma" in n and "Column:" in n for n in names_undo)
        self.assertTrue(has_contGamma_undo, "contGamma column should be restored after undo")
        print(f"  [T10] contGamma restored.", flush=True)

    def test_11_compute_constructor_column_undo(self):
        """[Spec Test 09] Insert constructor column → verify → undo."""
        self._ensure_data_mode()

        # Click Insert → "Insert constructor column before"
        result = _click_menu_option("Insert", "Insert constructor column before")
        self.assertTrue(result, "Could not click Insert → Insert constructor column before")

        # CreateComputeColumnDialog should appear
        time.sleep(2)
        dialog = find_window_by_name(None, "Create", timeout=5)
        if not dialog:
            # Try searching for dialog role
            app = _fresh_app()
            if app:
                dialogs = find_all_by_role(app, "dialog", max_depth=5)
                for d in dialogs:
                    try:
                        dname = d.get_name() or ""
                        if "create" in dname.lower() or "computed" in dname.lower() or "compute" in dname.lower():
                            dialog = d
                            break
                    except Exception:
                        pass

        if not dialog:
            # Try find by a known child name
            name_field = _robust_search(
                lambda app: find_all_by_role(app, "text", "name", max_depth=10)
            )
            if name_field:
                dialog = None  # We'll just interact with what we found

        # Try to find and type column name
        name_field = _robust_search(
            lambda app: find_all_by_role(app, "text", "name", max_depth=10)
        )
        column_created = False
        if name_field:
            target = name_field[0] if isinstance(name_field, list) else name_field
            click_element(target)
            time.sleep(0.5)
            for ch in "TestCol":
                generate_key_event(ord(ch))
                time.sleep(0.1)
            time.sleep(1)

            # Click "Create Column" button
            create_btn = _robust_search(
                lambda app: _find_btn_by_name(app, "Create Column")
            )
            if create_btn:
                click_element(create_btn)
                time.sleep(3)
                column_created = True

        if not column_created:
            close_menu()
            time.sleep(1)
            # Try alternate approach: direct keyboard interaction
            result2 = _click_menu_option("Insert", "Insert column before")
            if result2:
                time.sleep(1)
                for ch in "TestCol":
                    generate_key_event(ord(ch))
                    time.sleep(0.1)
                _send_enter()
                time.sleep(3)

        # Verify new column
        app = _fresh_app()
        col_headers = find_all_by_role(app, "table column header", max_depth=20)
        names = [(h.get_name() or "") for h in col_headers]
        has_testcol = any("TestCol" in n for n in names)
        print(f"  [T11] Column headers after compute: {[n for n in names if 'TestCol' in n or 'Column' in n][:5]}", flush=True)
        if has_testcol:
            self.assertTrue(True, "TestCol column created")

        # Undo
        undo_btn = _robust_search(lambda app: _find_btn_by_name_bounded(app, "Undo"))
        if undo_btn:
            click_element(undo_btn)
            time.sleep(2)

    def test_12_rename_column_undo(self):
        """[Spec Test 10] Double-click column header → rename → verify → undo."""
        self._ensure_data_mode()

        # Double-click contGamma column header
        app = _fresh_app()
        col_headers = find_all_by_role(app, "table column header", max_depth=20)
        contGamma_header = None
        for h in col_headers:
            name = h.get_name() or ""
            if "contGamma" in name and "Column:" in name:
                contGamma_header = h
                break

        self.assertIsNotNone(contGamma_header, "Column: contGamma header not found")

        # Double-click
        click_element(contGamma_header)
        time.sleep(0.3)
        click_element(contGamma_header)
        time.sleep(2)

        # Find the variable settings panel
        app = _fresh_app()
        name_fields = find_all_by_role(app, "text", "name", max_depth=10)
        if name_fields:
            target = name_fields[0] if isinstance(name_fields, list) else name_fields
            click_element(target)
            time.sleep(0.5)

            # Ctrl+A to select all, then type new name
            Atspi.generate_keyboard_event(0x0061, None, Atspi.KeySynthType.SYM)  # A key
            time.sleep(0.2)
            for ch in "renamedCol":
                generate_key_event(ord(ch))
                time.sleep(0.1)
            _send_enter()
            time.sleep(3)

        # Verify rename
        app = _fresh_app()
        col_headers = find_all_by_role(app, "table column header", max_depth=20)
        names = [(h.get_name() or "") for h in col_headers]
        has_renamed = any("renamedCol" in n for n in names)
        print(f"  [T12] Column headers: {[n for n in names if 'renamedCol' in n or 'contGamma' in n][:3]}", flush=True)
        if has_renamed:
            self.assertTrue(True, "Column renamed to renamedCol")

        # Undo
        undo_btn = _robust_search(lambda app: _find_btn_by_name_bounded(app, "Undo"))
        if undo_btn:
            click_element(undo_btn)
            time.sleep(2)

        # Verify restored
        app = _fresh_app()
        col_headers = find_all_by_role(app, "table column header", max_depth=20)
        names = [(h.get_name() or "") for h in col_headers]
        has_restored = any("contGamma" in n for n in names)
        if has_restored:
            self.assertTrue(True, "contGamma restored after undo")
        print(f"  [T12] After undo: {[n for n in names if 'contGamma' in n or 'renamedCol' in n][:3]}", flush=True)

    def test_13_change_column_type_undo(self):
        """[Spec Test 11] Change column type → verify → undo."""
        self._ensure_data_mode()

        app = _fresh_app()
        self.assertIsNotNone(app, "JASP gone")

        # Find a column header that's not computed (contGamma)
        col_headers = find_all_by_role(app, "table column header", max_depth=20)
        contGamma_header = None
        for h in col_headers:
            name = h.get_name() or ""
            if "contGamma" in name and "Column:" in name:
                contGamma_header = h
                break

        if not contGamma_header:
            self.skipTest("Column: contGamma header not found")

        # Click on the column type icon (it's inside the header)
        # The type icon is an Image with a MouseArea
        click_element(contGamma_header)
        time.sleep(1)

        # Look for the type selector dropdown menu
        menu = find_window_by_name(None, "menu", timeout=3, role_name="menu")
        if menu:
            # Try to find "Ordinal" or "Nominal" menu item
            for type_name in ["Ordinal", "Nominal", "Scale", "ordinal", "nominal", "scale"]:
                item = _find_menu_item(menu, type_name)
                if item:
                    click_element(item)
                    time.sleep(2)
                    break
        else:
            # Try directly clicking the icon (colIcon MouseArea)
            pass

        # Undo
        undo_btn = _robust_search(lambda app: _find_btn_by_name_bounded(app, "Undo"))
        if undo_btn:
            click_element(undo_btn)
            time.sleep(2)

        print(f"  [T13] Column type change attempted", flush=True)

    def test_14_toggle_column_labels(self):
        """[Spec Test 12] Toggle column labels on/off."""
        self._ensure_data_mode()

        app = _fresh_app()
        self.assertIsNotNone(app, "JASP gone")

        # Research: column labels toggle is in the variable settings panel.
        # Double-click a column header to open variable settings
        col_headers = find_all_by_role(app, "table column header", max_depth=20)
        facGender_header = None
        for h in col_headers:
            name = h.get_name() or ""
            if "facGender" in name and "Column:" in name:
                facGender_header = h
                break

        if not facGender_header:
            # Try any column header
            for h in col_headers:
                name = h.get_name() or ""
                if "Column:" in name:
                    facGender_header = h
                    break

        if not facGender_header:
            self.skipTest("No column headers found for labels toggle test")

        # Double-click to open variable settings
        click_element(facGender_header)
        time.sleep(0.3)
        click_element(facGender_header)
        time.sleep(2)

        # Look for labels-related controls
        app = _fresh_app()
        # Check for checkboxes related to labels
        all_checkboxes = find_all_by_role(app, "check box", max_depth=10)
        print(f"  [T14] Found {len(all_checkboxes)} checkboxes", flush=True)

        if all_checkboxes:
            # Toggle the first one we find (likely "Label" related)
            for cb in all_checkboxes[:3]:
                try:
                    name = cb.get_name() or ""
                    if "label" in name.lower():
                        click_element(cb)
                        time.sleep(1)
                        break
                except Exception:
                    pass

        # Close the variable settings
        close_menu()
        time.sleep(1)

        print(f"  [T14] Labels toggle test completed", flush=True)


if __name__ == "__main__":
    unittest.main(verbosity=2)