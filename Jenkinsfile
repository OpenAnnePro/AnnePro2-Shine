pipeline {
  agent any
  stages {
    stage('Build') {
      parallel {
        stage('Build C15') {
          agent {
            label 'gcc-arm-none-eabi'
          }
          steps {
            sh '''make C15'''
            archiveArtifacts artifacts: 'build/annepro2-shine-C15.bin', followSymlinks: false
          }
        }

        stage('Build C18') {
          agent {
            label 'gcc-arm-none-eabi'
          }
          steps {
            sh '''make C18'''
            archiveArtifacts artifacts: 'build/annepro2-shine-C18.bin', followSymlinks: false
          }
        }
      }
    }
  }
}
