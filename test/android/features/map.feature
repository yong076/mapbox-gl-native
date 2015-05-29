Feature: Map feature

  Scenario: I start the app and wait for the map to load
    Then I wait for the "MainActivity" screen to appear
    Then I take a screenshot

  Scenario: I start the app and switch between all the styles
    Then I wait for the "MainActivity" screen to appear
    Then I select "Mapbox Streets" from "spinner_style"
    Then I take a screenshot
    Then I select "Emerald" from "spinner_style"
    Then I take a screenshot
    Then I select "Light" from "spinner_style"
    Then I take a screenshot
    Then I select "Dark" from "spinner_style"
    Then I take a screenshot
