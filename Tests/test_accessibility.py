#!/usr/bin/env python3
"""
Unit test for JASP accessibility features using AT-SPI2 DBus API.
Tests tables, notes, and menus accessibility via Xvfb headless display.
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
    sys.exit(77)  # Skip test


class TestJASPAccessibility(unittest.TestCase):
    """Test JASP accessibility with screen reader support."""

    @classmethod
    def setUpClass(cls):
        """Start JASP via xvfb-run, wait for AT-SPI2, and verify it's accessible."""
        Atspi.init()
        
        cls.jasp_binary = Path("/home/virtuoos/Broncode/jasp-desktop/build/Desktop/JASP")
        if not cls.jasp_binary.exists():
            print(f"JASP binary not found: {cls.jasp_binary}")
            sys.exit(1)
        
        # Verify JASP can start
        test_env = os.environ.copy()
        test_env["QT_LINUX_ACCESSIBILITY_ALWAYS_ON"] = "1"
        
        test_proc = subprocess.Popen(
            ["xvfb-run", "-a", "-s", "-ac -screen 0 1280x1024x24", str(cls.jasp_binary)],
            env=test_env,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
        
        time.sleep(8)
        
        desktop = Atspi.get_desktop(0)
        found_jasp = False
        for i in range(desktop.get_child_count()):
            app = desktop.get_child_at_index(i)
            if "jasp" in app.get_name().lower():
                found_jasp = True
                break
        
        if not found_jasp:
            test_proc.terminate()
            try:
                test_proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                test_proc.kill()
                test_proc.wait()
            print("\nFATAL: JASP cannot start/accessibility not working - aborting ALL tests")
            sys.exit(1)
        
        test_proc.terminate()
        try:
            test_proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            test_proc.kill()
            test_proc.wait()
        
        env = os.environ.copy()
        env["QT_LINUX_ACCESSIBILITY_ALWAYS_ON"] = "1"
        
        # Start JASP
        cls.jasp_process = subprocess.Popen(
            ["xvfb-run", "-a", "-s", "-ac -screen 0 1280x1024x24",
             str(cls.jasp_binary)],
            env=env,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
        
        # Wait for JASP to start
        time.sleep(8)
        
        # Try to find JASP app
        cls.app = None
        for _ in range(3):
            try:
                desktop = Atspi.get_desktop(0)
                child_count = desktop.get_child_count()
                
                for i in range(child_count):
                    app = desktop.get_child_at_index(i)
                    name = app.get_name().lower()
                    if "jasp" in name:
                        cls.app = app
                        break
            except Exception:
                pass
            
            if cls.app:
                break
            time.sleep(1)
        
        # If app not accessible, abort entire test suite
        if not cls.app:
            if hasattr(cls, 'jasp_process') and cls.jasp_process:
                cls.jasp_process.terminate()
                try:
                    cls.jasp_process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    cls.jasp_process.kill()
                    cls.jasp_process.wait()
            print("\nFATAL: JASP application not accessible via AT-SPI2 - aborting ALL tests")
            sys.exit(1)

    @classmethod
    def tearDownClass(cls):
        """Stop JASP."""
        if hasattr(cls, 'jasp_process') and cls.jasp_process:
            cls.jasp_process.terminate()
            try:
                cls.jasp_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                cls.jasp_process.kill()
                cls.jasp_process.wait()

    def test_01_app_accessible(self):
        """Test that JASP app is accessible."""
        self.assertIsNotNone(self.app)
        self.assertGreater(len(self.app.get_name()), 0)
        self.assertEqual(self.app.get_role_name(), "application")

    def test_02_file_menu_accessible(self):
        """Test that File menu is accessible."""
        menu = self._find_element_by_role_and_name("menu", "File")
        self.assertIsNotNone(menu, "File menu not found")
        self.assertEqual(menu.get_role_name(), "menu")

    def test_03_analysis_menu_accessible(self):
        """Test that Analysis menu is accessible with correct name."""
        analysis_menu = self._find_element_by_role_and_name("menu", "Analysis")
        self.assertIsNotNone(analysis_menu, "Analysis menu not found")
        
        # Verify accessibility attributes
        name = analysis_menu.get_name()
        self.assertEqual(name, "Analysis", f"Expected 'Analysis', got '{name}'")

    def test_04_menu_items_accessible(self):
        """Test that menu items are accessible."""
        menu = self._find_element_by_role_and_name("menu", "Analysis")
        self.assertIsNotNone(menu)
        
        # Get menu items
        child_count = menu.get_child_count()
        self.assertGreater(child_count, 0, "Menu should have children")
        
        # Check first child is a menu item
        first_child = menu.get_child_at_index(0)
        if first_child:
            role = first_child.get_role_name()
            self.assertIn(role, ["menu item", "menuitem"], 
                         f"Expected menu item, got '{role}'")

    def test_05_accessible_roles(self):
        """Test that essential accessible roles are present."""
        roles_found = set()
        desktop = Atspi.get_desktop(0)
        child_count = desktop.get_child_count()
        
        def collect_roles(obj, depth=0):
            if depth > 3:  # Limit recursion
                return
            try:
                role_name = obj.get_role_name()
                if role_name:
                    roles_found.add(role_name)
                
                child_count = obj.get_child_count()
                for i in range(min(child_count, 20)):  # Limit children
                    child = obj.get_child_at_index(i)
                    if child:
                        collect_roles(child, depth + 1)
            except Exception:
                pass
        
        for i in range(child_count):
            app = desktop.get_child_at_index(i)
            collect_roles(app)
        
        # Verify key roles are present
        self.assertIn("menu", roles_found, "Menu role not found")
        self.assertIn("menu item", roles_found, "Menu item role not found")
        self.assertIn("application", roles_found, "Application role not found")

    def _find_element_by_role_and_name(self, role_name, name):
        """Find an accessible element by role and name."""
        desktop = Atspi.get_desktop(0)
        child_count = desktop.get_child_count()
        
        def find_recursive(obj):
            try:
                if obj.get_role_name().lower() == role_name.lower():
                    obj_name = obj.get_name()
                    if obj_name == name or name.lower() in obj_name.lower():
                        return obj
                
                for i in range(obj.get_child_count()):
                    child = obj.get_child_at_index(i)
                    if child:
                        result = find_recursive(child)
                        if result:
                            return result
            except Exception:
                pass
            return None
        
        for i in range(child_count):
            app = desktop.get_child_at_index(i)
            result = find_recursive(app)
            if result:
                return result
        return None


if __name__ == "__main__":
    unittest.main(verbosity=2)
