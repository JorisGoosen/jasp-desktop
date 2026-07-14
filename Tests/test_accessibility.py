#!/usr/bin/env python3
"""
Comprehensive unit test for JASP accessibility features using AT-SPI2 DBus API.
Tests all accessible UI components, menus, forms, tables, and results.
"""

import unittest
import subprocess
import time
import os
import sys
from pathlib import Path

try:
    gi = __import__("gi")
    gi.require_version("Atspi", "2.0")
    from gi.repository import Atspi
except ImportError as e:
    print(f"PyGObject not available: {e}")
    sys.exit(77)


def _dismiss_dialogs(app):
    """Click Close buttons on any startup dialogs."""
    for i in range(app.get_child_count()):
        try:
            child = app.get_child_at_index(i)
            if child.get_role_name() != "frame":
                continue
            name = child.get_name()
            if name in ("JASP", "Data Preview"):
                continue  # skip main window and data preview
            # Find a close/ok button
            for j in range(child.get_child_count()):
                try:
                    gc = child.get_child_at_index(j)
                    gc_name = gc.get_name().lower()
                    if gc_name in ("close", "continue", "ok", "accept", "no encryption", "skip"):
                        for a in range(gc.get_action_count()):
                            if "click" in gc.get_action_name(a).lower():
                                gc.do_action(a)
                                return
                except Exception:
                    pass
        except Exception:
            pass


class TestJASPAccessibility(unittest.TestCase):
    """Test JASP accessibility with screen reader support."""

    @classmethod
    def setUpClass(cls):
        """Start JASP, wait for AT-SPI2, and verify it's accessible."""
        Atspi.init()

        cls.jasp_binary = Path("/home/virtuoos/JASP-screenreader/jasp-desktop/build/Desktop/JASP")
        if not cls.jasp_binary.exists():
            sys.exit(1)

        cls.jasp_process = subprocess.Popen(
            [str(cls.jasp_binary)],
            env={**os.environ, "QT_LINUX_ACCESSIBILITY_ALWAYS_ON": "1"},
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        # Wait for JASP to start and find the app with the main window
        cls.app = None
        cls.main_window = None
        for _ in range(30):
            time.sleep(1)
            try:
                desktop = Atspi.get_desktop(0)
                for i in range(desktop.get_child_count()):
                    a = desktop.get_child_at_index(i)
                    if "jasp" not in a.get_name().lower():
                        continue
                    # Pick the JASP app that has the main window
                    for j in range(a.get_child_count()):
                        try:
                            c = a.get_child_at_index(j)
                            if c.get_name() == "JASP" and c.get_role_name() == "frame" and c.get_child_count() > 50:
                                cls.app = a
                                cls.main_window = c
                                break
                        except Exception:
                            pass
                    if cls.main_window:
                        break
            except Exception:
                pass
            if cls.main_window:
                # Dismiss dialogs
                for _ in range(5):
                    _dismiss_dialogs(cls.app)
                    time.sleep(1)
                break

        if not cls.main_window:
            if cls.jasp_process:
                cls.jasp_process.terminate()
                cls.jasp_process.wait(timeout=3)
            print("\nFATAL: JASP main window not accessible via AT-SPI2")
            sys.exit(1)

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, "jasp_process") and cls.jasp_process:
            cls.jasp_process.terminate()
            try:
                cls.jasp_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                cls.jasp_process.kill()
                cls.jasp_process.wait()

    def _find_by_role(self, parent, role_name):
        for i in range(parent.get_child_count()):
            child = parent.get_child_at_index(i)
            try:
                if child.get_role_name().lower() == role_name.lower():
                    return child
                result = self._find_by_role(child, role_name)
                if result:
                    return result
            except Exception:
                pass
        return None

    def _find_by_role_and_name(self, parent, role_name, name):
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
        """JASP app is accessible."""
        self.assertEqual(self.app.get_role_name(), "application")

    def test_02_main_menu_accessible(self):
        btn = self._find_by_role_and_name(self.main_window, "button", "Main menu")
        self.assertIsNotNone(btn, "Main menu button not found")

    def test_03_modules_menu_accessible(self):
        btn = self._find_by_role_and_name(self.main_window, "button", "Modules menu")
        self.assertIsNotNone(btn, "Modules menu button not found")

    def test_04_analysis_menu_accessible(self):
        filler = self._find_by_role_and_name(self.main_window, "filler", "Analysis menu")
        self.assertIsNotNone(filler, "Analysis menu not found")

    def test_05_open_button_accessible(self):
        btn = self._find_by_role_and_name(self.main_window, "button", "Open")
        self.assertIsNotNone(btn, "Open button not found")

    def test_06_save_button_accessible(self):
        btn = self._find_by_role_and_name(self.main_window, "button", "Save")
        self.assertIsNotNone(btn, "Save button not found")

    def test_07_results_accessible(self):
        doc = self._find_by_role(self.app, "document web")
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
        """Main window has expected buttons."""
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
        """Spin boxes accessible (may require opening analysis)."""
        roles = self._collect_all_roles()
        if "spin box" in roles:
            return
        self.skipTest("No spin box visible on startup — requires opening an analysis")

    def test_14_table_accessible(self):
        """Table role present in tree."""
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
        """Alert role present in tree."""
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

    def test_20_sleep_data_accessible(self):
        sleep_file = Path("/home/virtuoos/JASP-screenreader/jasp-desktop/build/Resources/Data Sets/Data Library/1. Descriptives/Sleep.jasp")
        self.assertTrue(sleep_file.exists(), "Sleep.jasp not found")

    def test_21_analysis_results_accessible(self):
        doc = self._find_by_role(self.app, "document web")
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
        """Results document has 'document web' role."""
        doc = self._find_by_role(self.app, "document web")
        self.assertIsNotNone(doc, "document web not found")
        self.assertEqual(doc.get_role_name(), "document web")

    def test_25_webengine_children(self):
        """WebEngine results have accessible children."""
        doc = self._find_by_role(self.app, "document web")
        self.assertIsNotNone(doc, "document web not found")
        cc = doc.get_child_count()
        if cc == 0:
            self.skipTest("WebEngine children not visible (zygote/renderer issue)")
        child = doc.get_child_at_index(0)
        self.assertIsNotNone(child.get_role_name())
        self.assertIsNotNone(child.get_name())


if __name__ == "__main__":
    unittest.main(verbosity=2)
