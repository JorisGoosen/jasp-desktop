#!/usr/bin/env python3
"""Open debug.csv via File Menu → Open → Data Library using AT-SPI2."""

import unittest
import time
import sys
from accessibility_common import (
    Atspi, find_jasp_app, click_element, close_menu,
    find_all_by_role, dismiss_dialogs, get_jasp_app,
    find_window_by_name,
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


if __name__ == "__main__":
    unittest.main(verbosity=2)