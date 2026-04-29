import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_section"
    when: windowShown

    Section {
        id: section
        title: "Test Section"
    }

    function test_section_title() {
        compare(section.title, "Test Section", "Section title should match")
    }
}
