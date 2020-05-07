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
import datetime
import os
import sys

try:
    import ctools

except ImportError:
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    sys.path.insert(0, project_root)

    import ctools


def get_version():
    import ctools

    return ctools.__version__


# -- Project information -----------------------------------------------------
build_at = datetime.date.today().strftime("%a %b %d %Y")
project = "CTools"
copyright = "2019, ko-han.    Last updated on " + build_at
author = "ko-han"

# The full version, including alpha/beta/rc tags
version = get_version()
release = get_version()

# -- General configuration ---------------------------------------------------
master_doc = "index"

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
    "numpydoc",
    "sphinx_copybutton",
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

pygments_style = "sphinx"

rst_epilog = """\n
.. |ProjectVersion| replace:: {version}
.. |ProjectName| replace:: {project}
.. |BuildAt| replace:: {build_at}
""".format(
    project=project, version=version, build_at=build_at,
)

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "alabaster"

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ["_static"]

html_title = "%s v%s Manual" % (project, version)

font_family = '"Inconsolata", monospace, monospace'
description = (
    "A collection of useful data structures and functions written in C for Python."
)
html_theme_options = {
    "github_user": "ko-han",
    "github_repo": "python-ctools",
    "github_banner": "false",
    "github_button": "true",
    "description": description,
    "fixed_sidebar": "true",
    "show_relbars": "true",
    "code_font_family": font_family,
    "font_family": font_family,
    "head_font_family": font_family,
}

# -- sphinx.ext.autodoc ------------------------------------------------------
autodoc_docstring_signature = True
autodoc_default_flags = ["members"]
autoclass_content = "class"
autosummary_generate = True

numpydoc_show_class_members = False
numpydoc_class_members_toctree = True
numpydoc_xref_param_type = True

intersphinx_cache_limit = 6
intersphinx_mapping = {
    "python": ("https://docs.python.org/3/", None),
}

copybutton_prompt_text = ">>> "
