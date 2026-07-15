#!/usr/bin/env python3
"""Standalone test: open debug.csv via JASP FileMenu → Data Library using AT-SPI2."""

import unittest
import time
import sys
import os
from accessibility_common import (
    Atspi, find_jasp_app, click_element, close_menu,
    find_all_by_role, dismiss_dialogs, get_jasp_app,
)


def _find_buttons_with_names(app):
    """Find all buttons AND capture their names during traversal (before refs go stale)."""
    results = []
    try:
        for i in range(min(app.get_child_count(), 100)):
            _traverse(app.get_child_at_index(i), results, 0, 6)
    except Exception:
        pass
    return results


def _traverse(obj, results, depth, max_depth):
    if depth > max_depth:
        return
    try:
        role = obj.get_role_name()
        if "button" in role.lower():
            name = obj.get_name()
            results.append((name, obj))
    except Exception:
        return
    try:
        for i in range(min(obj.get_child_count(), 60)):
            _traverse(obj.get_child_at_index(i), results, depth + 1, max_depth)
    except Exception:
        pass


def _find_by_role_and_name(parent, role_name, name):
    try:
        if parent.get_role_name().lower() == role_name.lower():
            pname = parent.get_name().lower()
            if name.lower() in pname or pname == name.lower():
                return parent
    except Exception:
        return None
    try:
        for i in range(parent.get_child_count()):
            child = parent.get_child_at_index(i)
            try:
                result = _find_by_role_and_name(child, role_name, name)
                if result:
                    return result
            except Exception:
                pass
    except Exception:
        pass
    return None
    try:
        if parent.get_role_name().lower() == role_name.lower():
            pname = parent.get_name().lower()
            if name.lower() in pname or pname == name.lower():
                return parent
    except Exception:
        return None
    try:
        for i in range(parent.get_child_count()):
            child = parent.get_child_at_index(i)
            try:
                result = _find_by_role_and_name(child, role_name, name)
                if result:
                    return result
            except Exception:
                pass
    except Exception:
        pass
    return None


def _fresh_app():
    try:
        desktop = Atspi.get_desktop(0)
        best, best_cc = None, -1
        for i in range(desktop.get_child_count()):
            a = desktop.get_child_at_index(i)
            if "jasp" in a.get_name().lower():
                cc = a.get_child_count()
                if cc > best_cc:
                    best_cc, best = cc, a
        return best
    except Exception:
        return None


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

    def _open_file_menu_and_click(self, action_name):
        """Open file menu, click an action button, return fresh app."""
        try:
            app, mw = find_jasp_app(timeout=5)
            if not mw:
                return None
            btn = _find_by_role_and_name(mw, "button", "Main menu")
            if not btn:
                return None
            click_element(btn)
            time.sleep(5)
            time.sleep(1)
            app = _fresh_app()
            if not app:
                return None
            target = _find_by_role_and_name(app, "button", action_name)
            if not target:
                return None
            click_element(target)
            time.sleep(2)
            return app
        except Exception:
            return None

    def _ensure_qt_file_dialogs(self):
        """Disable native file dialogs via Preferences UI."""
        app = self._open_file_menu_and_click("Preferences")
        if not app:
            return False
        app = _fresh_app()
        if not app:
            return False
        all_btns = find_all_by_role(app, "button") or find_all_by_role(app, "push button", max_depth=8)
        ui_names = [(b.get_name(), b) for b in all_btns
                    if "ui" in b.get_name().lower() or "interface" in b.get_name().lower()]
        if not ui_names:
            close_menu()
            return False
        click_element(ui_names[0][1])
        time.sleep(2)

        app = _fresh_app()
        if not app:
            return False
        all_checks = find_all_by_role(app, "check box", max_depth=8) or find_all_by_role(app, "push button", max_depth=8)
        native = [b for b in all_checks if "native" in b.get_name().lower()]
        if not native:
            close_menu()
            return False
        click_element(native[0])
        time.sleep(1)
        close_menu()
        time.sleep(1)
        return True

    def test_01_disable_native_dialogs(self):
        self.assertTrue(self._ensure_qt_file_dialogs(),
                        "Could not disable native file dialogs via Preferences")

    def test_02_open_debug_via_data_library(self):
        app = self._open_file_menu_and_click("Open")
        self.assertIsNotNone(app, "Could not open file menu and click Open")

        app = _fresh_app()
        self.assertIsNotNone(app, "JASP gone after clicking Open")
        data_lib = _find_by_role_and_name(app, "button", "Data Library")
        self.assertIsNotNone(data_lib, "Data Library tab not found")
        click_element(data_lib)
        time.sleep(3)

        app = _fresh_app()
        self.assertIsNotNone(app, "JASP gone after clicking Data Library")

        named = _find_buttons_with_names(app)
        print(f"\n  [DL] {len(named)} named buttons, first 30: {sorted(set(n for n,_ in named))[:30]}", flush=True)

        debug_entry = None
        for name, entry in named:
            if "debug" in name.lower():
                debug_entry = entry
                break
        self.assertIsNotNone(debug_entry,
            f"debug.csv not found. Named: {[n[:30] for n,_ in named[:25]]}")
        click_element(debug_entry)
        time.sleep(5)

        close_menu()
        dismiss_dialogs(self.app)
        time.sleep(2)

        app, mw = find_jasp_app(timeout=15, main_window_names=("JASP", "debug"))
        self.assertIsNotNone(mw, "JASP window not found after loading debug.csv")

        all_btns = find_all_by_role(app, "button", max_depth=8) or find_all_by_role(app, "push button", max_depth=8)
        btn_names = [b.get_name().lower() for b in all_btns]
        self.assertTrue(any("edit data" in n for n in btn_names),
                        "Edit Data button not found after CSV load")

        edit_btn = _find_by_role_and_name(mw, "button", "Edit Data")
        if not edit_btn:
            edit_btn = _find_by_role_and_name(mw, "push button", "Edit Data")
        self.assertIsNotNone(edit_btn, "Edit Data button not found")
        click_element(edit_btn)
        time.sleep(3)

        app = _fresh_app()
        all_btns = find_all_by_role(app, "button", max_depth=8) or find_all_by_role(app, "push button", max_depth=8)
        data_names = [b.get_name().lower() for b in all_btns]
        expected = ["analyses", "synchronisation", "resize data", "insert", "remove", "undo", "redo"]
        found = [n for n in expected if any(n in bn for bn in data_names)]
        self.assertGreater(len(found), 3, f"Too few data-mode buttons: {found}")


if __name__ == "__main__":
    unittest.main(verbosity=2)