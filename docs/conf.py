# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys

docdir = os.path.abspath(".")

sys.path.insert(0, os.path.abspath("sphinx"))

# Interpret `single-backticks` to be a cross-reference to any kind of
# referenceable object. Unresolvable or ambiguous references will emit a
# warning at build time.
default_role = 'any'

# -- Project information -----------------------------------------------------

project = 'libvfn'
copyright = '2022, The libvfn Authors'
author = 'The libvfn Authors'

# The full version, including alpha/beta/rc tags
release = '0.0.99'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ['kerneldoc', 'depfile']

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

highlight_language = 'none'

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

html_css_files = [
    'theme_overrides.css',
]

kerneldoc_bin = os.path.join(docdir, '../scripts/kernel-doc')
kerneldoc_srctree = os.path.join(docdir, '..')
