#!/usr/bin/env python3
"""Open a CSV via File Menu → Open → Data Library → Descriptives → Sleep CSV."""

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
        for i in range(min(parent.get_child_count(), 100)):
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

    def test_open_csv_from_data_library(self):
        close_menu()
        time.sleep(1)
        self.assertTrue(self._open_file_menu(), "Could not open file menu")

        open_btns = _robust_search(
            lambda app: find_all_by_role(app, "button", "open", max_depth=8)
        )
        target = open_btns[-1] if len(open_btns) >= 2 else open_btns[0] if open_btns else None
        self.assertIsNotNone(target, "No Open button in file menu")
        click_element(target)
        time.sleep(2)

        dl_btn = _robust_search(
            lambda app: _find_btn_by_name(app, "Data Library")
        )
        self.assertIsNotNone(dl_btn, "Data Library tab not found")
        click_element(dl_btn)
        time.sleep(3)

        folder = _robust_search(
            lambda app: _find_btn_by_name(app, "Folder 1. Descriptives")
        )
        self.assertIsNotNone(folder, "Folder 1. Descriptives not found")
        click_element(folder)
        time.sleep(3)

        file_btn = _robust_search(
            lambda app: _find_btn_by_name(app, "Datafile Sleep")
        )
        self.assertIsNotNone(file_btn, "Datafile Sleep not found")
        click_element(file_btn)
        time.sleep(5)

        dp = find_window_by_name(None, "Data Preview", timeout=10)
        self.assertIsNotNone(dp, "Data Preview not found after selecting dataset")

        load_btn = _find_btn_by_name(dp, "Load")
        self.assertIsNotNone(load_btn, "Load button not found")
        click_element(load_btn)
        time.sleep(5)

        close_menu()
        time.sleep(2)

        app = _fresh_app()
        self.assertIsNotNone(app, "JASP gone after Load")
        all_btns = find_all_by_role(app, "button", max_depth=8)
        names = [(b.get_name() or "").lower() for b in all_btns]
        self.assertTrue(any("edit data" in n or "sync data" in n for n in names),
                        "No data-mode buttons found after loading")


if __name__ == "__main__":
    unittest.main(verbosity=2)