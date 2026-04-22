# Product Definition

## Project Name

EduceLab Core

## Description

A C++ utility library providing common types and utilities for EduceLab projects.

## Problem Statement

Developers across EduceLab projects need to reimplement common types and utilities repeatedly, and there is no shared foundation for EduceLab C++ projects to build upon.

## Target Users

EduceLab developers building C++ applications and libraries.

## Key Goals

1. Provide a stable, reusable foundation for all EduceLab C++ projects
2. Offer well-tested, header-friendly types (Vec, Mat, Image, Mesh, UVMap, etc.)
3. Minimize external dependencies while maximizing utility
4. Provide standard Mesh IO (OBJ and PLY read/write) including UV maps, texture
   paths, and compile-time vertex trait detection (`has_normal`, `has_color`)

## Current Version

v0.3.0
