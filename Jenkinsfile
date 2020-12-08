pipeline {
  agent any
  stages {
    stage('Check style') {
      agent {
        label 'gcc-arm-none-eabi'
      }
      steps {
        sh """make clang-format-ci"""
      }
    }

    stage('Build') {
      parallel {
        stage('Build C15') {
          agent {
            label 'gcc-arm-none-eabi'
          }
          steps {
            sh '''make C15'''
            sh '''
              cd build
              tar czvf c15_archive.tgz C15
              cd ..
            '''
            archiveArtifacts artifacts: 'build/C15/*.bin', followSymlinks: false
            archiveArtifacts artifacts: 'build/*.tgz', followSymlinks: false
            fingerprint 'build/C15/*.elf'
            fingerprint 'build/C15/*.bin'
          }
        }

        stage('Build C18') {
          agent {
            label 'gcc-arm-none-eabi'
          }
          steps {
            sh '''make C18'''
            sh '''
              cd build
              tar czvf c18_archive.tgz C18
              cd ..
            '''
            archiveArtifacts artifacts: 'build/C18/*.bin', followSymlinks: false
            archiveArtifacts artifacts: 'build/*.tgz', followSymlinks: false
            fingerprint 'build/C18/*.elf'
            fingerprint 'build/C18/*.bin'
          }
        }
      }
    }
  }
}
