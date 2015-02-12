#!/usr/bin/env bash
../gradlew -b build.gradle clean assembleRelease uploadArchives -PnexusUsername="${SONATYPE_USERNAME}" -PnexusPassword="${SONATYPE_PASSWORD}"
