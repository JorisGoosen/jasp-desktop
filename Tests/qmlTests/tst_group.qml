import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_group"
    when: windowShown

    Group {
        id: group
        title: "Test Group"
    }

    function test_group_title() {
        compare(group.title, "Test Group", "Group title should match")
    }
}
