#!/usr/bin/env python3
"""
Comprehensive unit test for JASP accessibility features using AT-SPI2 DBus API.
Tests all accessible UI components, menus, forms, tables, and results.

JASP is started by the run_accessibility_tests.sh runner; this script only connects.
"""

import unittest
import time
import sys
from accessibility_common import (
    Atspi, find_jasp_app, find_document_web, dismiss_dialogs,
)


def _find_by_role(parent, role_name):
    for i in range(parent.get_child_count()):
        child = parent.get_child_at_index(i)
        try:
            if child.get_role_name().lower() == role_name.lower():
                return child
            result = _find_by_role(child, role_name)
            if result:
                return result
        except Exception:
            pass
    return None


def _find_by_role_and_name(parent, role_name, name):
    for i in range(parent.get_child_count()):
        child = parent.get_child_at_index(i)
        try:
            if child.get_role_name().lower() == role_name.lower():
                cname = child.get_name().lower()
                if name.lower() in cname or cname == name.lower():
                    return child
        except Exception:
            pass
    return None


class TestJASPAccessibility(unittest.TestCase):
    """Test JASP accessibility with screen reader support."""

    @classmethod
    def setUpClass(cls):
        """Connect to already-running JASP via AT-SPI2."""
        Atspi.init()

        cls.app, cls.main_window = find_jasp_app(timeout=30)
        if not cls.main_window:
            sys.exit(1)

        for _ in range(5):
            dismiss_dialogs(cls.app)
            time.sleep(1)

    def _get_all_accessible_elements(self, obj, depth=0, elements=None):
        if elements is None:
            elements = []
        if depth > 5:
            return elements
        try:
            elements.append({"role": obj.get_role_name() or "unknown", "name": obj.get_name() or ""})
            cc = obj.get_child_count()
            if cc > 0 and depth > 3:
                cc = min(cc, 15)
            else:
                cc = min(cc, 60)
            for i in range(cc):
                try:
                    child = obj.get_child_at_index(i)
                    if child:
                        self._get_all_accessible_elements(child, depth + 1, elements)
                except Exception:
                    pass
        except Exception:
            pass
        return elements

    def _collect_all_roles(self):
        roles = set()
        desktop = Atspi.get_desktop(0)
        for i in range(desktop.get_child_count()):
            app = desktop.get_child_at_index(i)
            elements = self._get_all_accessible_elements(app)
            for e in elements:
                if e["role"]:
                    roles.add(e["role"].lower())
        return roles

    def _count_components(self):
        counts = {"buttons": 0, "fillers": 0, "menus": 0, "menu_items": 0,
                  "text": 0, "labels": 0, "spin_boxes": 0, "combo_boxes": 0,
                  "tables": 0, "documents": 0, "frames": 0, "panels": 0}
        elements = self._get_all_accessible_elements(self.main_window)
        for e in elements:
            r = e["role"].lower() if e["role"] else ""
            if "button" in r: counts["buttons"] += 1
            elif "filler" in r: counts["fillers"] += 1
            elif "menu item" in r: counts["menu_items"] += 1
            elif "menu" in r: counts["menus"] += 1
            elif "text" in r: counts["text"] += 1
            elif "label" in r: counts["labels"] += 1
            elif "spin" in r: counts["spin_boxes"] += 1
            elif "combo" in r: counts["combo_boxes"] += 1
            elif "table" in r: counts["tables"] += 1
            elif "document" in r: counts["documents"] += 1
            elif "frame" in r: counts["frames"] += 1
            elif "panel" in r: counts["panels"] += 1
        return counts

    # --- tests ---

    def test_01_app_accessible(self):
        self.assertEqual(self.app.get_role_name(), "application")

    def test_02_main_menu_accessible(self):
        btn = _find_by_role_and_name(self.main_window, "button", "Main menu")
        self.assertIsNotNone(btn, "Main menu button not found")

    def test_03_modules_menu_accessible(self):
        btn = _find_by_role_and_name(self.main_window, "button", "Modules menu")
        self.assertIsNotNone(btn, "Modules menu button not found")

    def test_04_analysis_menu_accessible(self):
        filler = _find_by_role_and_name(self.main_window, "filler", "Analysis menu")
        self.assertIsNotNone(filler, "Analysis menu not found")

    def test_05_open_button_accessible(self):
        btn = _find_by_role_and_name(self.main_window, "button", "Open")
        self.assertIsNotNone(btn, "Open button not found")

    def test_06_save_button_accessible(self):
        btn = _find_by_role_and_name(self.main_window, "button", "Save")
        self.assertIsNotNone(btn, "Save button not found")

    def test_07_results_accessible(self):
        doc = find_document_web(self.app)
        self.assertIsNotNone(doc, "Results document not found")

    def test_08_data_panel_accessible(self):
        data_frame = None
        for i in range(self.app.get_child_count()):
            try:
                c = self.app.get_child_at_index(i)
                if c.get_name() == "Data Preview":
                    data_frame = c
                    break
            except Exception:
                pass
        self.assertIsNotNone(data_frame, "Data panel not accessible")

    def test_09_main_window_structure(self):
        buttons = []
        for i in range(self.main_window.get_child_count()):
            try:
                c = self.main_window.get_child_at_index(i)
                if "button" in c.get_role_name().lower():
                    buttons.append(c.get_name())
            except Exception:
                pass
        expected = ["Main menu", "Open", "Save", "Modules menu"]
        for name in expected:
            self.assertIn(name, buttons, f"'{name}' button not found")

    def test_10_accessible_roles_present(self):
        roles = self._collect_all_roles()
        required = {"application", "button", "filler", "frame", "panel",
                     "text", "check box", "separator", "label"}
        for role in required:
            self.assertIn(role, roles, f"Role '{role}' not found")

    def test_11_menu_items_accessible(self):
        roles = self._collect_all_roles()
        self.assertTrue(any(r in roles for r in ("filler", "button", "menu")),
                        "No menu-related roles found")

    def test_12_accessible_names(self):
        elements = self._get_all_accessible_elements(self.main_window)
        named = [e for e in elements if e["name"]]
        self.assertGreater(len(named), 0, "No named elements in main window")
        btn_names = [e["name"] for e in elements if "button" in e["role"] and e["name"]]
        self.assertGreater(len(btn_names), 0, "Buttons without names")

    def test_13_spin_box_accessible(self):
        roles = self._collect_all_roles()
        if "spin box" in roles:
            return
        self.skipTest("No spin box visible on startup — requires opening an analysis")

    def test_14_table_accessible(self):
        roles = self._collect_all_roles()
        if "table" in roles:
            return
        self.skipTest("No table visible on startup — requires loading data")

    def test_15_document_accessible(self):
        roles = self._collect_all_roles()
        self.assertTrue("document" in roles or "document web" in roles,
                        "Document components not accessible")

    def test_16_window_accessible(self):
        self.assertIsNotNone(self.main_window, "Main window not accessible")
        self.assertEqual(self.main_window.get_role_name(), "frame")
        self.assertGreater(self.main_window.get_child_count(), 50, "Main window too shallow")

    def test_17_alert_messages_accessible(self):
        roles = self._collect_all_roles()
        if "alert" in roles:
            return
        self.skipTest("No alert visible on startup")

    def test_18_accessible_tree_depth(self):
        elements = self._get_all_accessible_elements(self.main_window)
        self.assertGreater(len(elements), 30, f"Only {len(elements)} elements in tree")

    def test_19_component_counts(self):
        counts = self._count_components()
        self.assertGreater(counts["buttons"], 5, f"Only {counts['buttons']} buttons")
        self.assertGreater(counts["frames"], 0, "No frames")
        self.assertGreater(counts["text"], 0, "No text elements")

    def test_20_sleep_data_exists(self):
        import accessibility_common
        sleep_file = accessibility_common.repo_root() / "jasp-desktop" / "build" / "Resources" / "Data Sets" / "Data Library" / "1. Descriptives" / "Sleep.jasp"
        self.assertTrue(sleep_file.exists(), f"Sleep.jasp not found at {sleep_file}")

    def test_21_analysis_results_accessible(self):
        doc = find_document_web(self.app)
        self.assertIsNotNone(doc, "Results document not accessible")
        self.assertIsNotNone(doc.get_name())

    def test_22_rich_text_accessible(self):
        elements = self._get_all_accessible_elements(self.main_window)
        text_el = [e for e in elements if "text" in e["role"] or "label" in e["role"]]
        self.assertGreater(len(text_el), 0, "No text or label components")

    def test_23_form_controls_accessible(self):
        roles = self._collect_all_roles()
        found = [r for r in ("spin box", "check box", "combo box", "text") if r in roles]
        self.assertGreater(len(found), 0, "No form controls")

    def test_24_webengine_document_role(self):
        doc = find_document_web(self.app)
        self.assertIsNotNone(doc, "document web not found")
        self.assertEqual(doc.get_role_name(), "document web")

    def test_25_webengine_children(self):
        doc = find_document_web(self.app)
        self.assertIsNotNone(doc, "document web not found")
        cc = doc.get_child_count()
        if cc == 0:
            self.skipTest("WebEngine children not yet loaded — run help/results tests for content")
        child = doc.get_child_at_index(0)
        self.assertIsNotNone(child.get_role_name())
        self.assertIsNotNone(child.get_name())


if __name__ == "__main__":
    unittest.main(verbosity=2)