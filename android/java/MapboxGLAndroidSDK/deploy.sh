#!/usr/bin/env bash
../gradlew -b build.gradle clean assembleRelease uploadArchives -PnexusUsername=$SONATYPE_USERNAME -PnexusPassword=$SONATYPE_PASSWORD -Psigning.keyId=$SIGNING_KEYID -Psigning.password=$SIGNING_PASSWORD -Psigning.secretKeyRingFile=s3://mapbox/mapbox-gl-native/android/keys/mapbox-android-key.gpg
