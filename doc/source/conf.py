# -*- coding: utf-8 -*-
import os
import sys
import subprocess
import datetime

# Important: keep this as a string or sphinx will fail with
# a non-helpful error message
TABLES_VERSION = "20211110"

# -- General configuration -----------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
needs_sphinx = "3.2.0"

# Add any Sphinx extension module names here, as strings. They can be extensions
# coming with Sphinx (named 'sphinx.ext.*') or your custom ones.
extensions = [
    "breathe",
    "sphinx.ext.mathjax",
    "sphinx.ext.graphviz",
    "sphinxcontrib.googleanalytics",
    "sphinx_sitemap",
]

# Configure Breathe.
# When building with CMake, the path to doxyxml is passed via the command line.
breathe_projects = {"tables": "doxyxml"}
breathe_default_project = "tables"
breathe_domain_by_extension = {"h": "cpp", "hpp": "cpp"}

highlight_language = "c++"
primary_domain = "cpp"

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# The suffix of source filenames.
source_suffix = ".rst"

# The encoding of source files.
# source_encoding = 'utf-8-sig'

# The master toctree document.
master_doc = "index"

# General information about the project.
project = "ampl::tables"
copyright = "2015-{}, AMPL Optimization Inc".format(datetime.date.today().year)

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.
#
# The short X.Y version.
version = TABLES_VERSION

# The full version, including alpha/beta/rc tags.
release = TABLES_VERSION

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
exclude_patterns = ["_build", "virtualenv"]

# The reST default role (used for this markup: `text`) to use for all documents.
default_role = "cpp:any"

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = "sphinx"

# A list of ignored prefixes for module index sorting.
# modindex_common_prefix = []


# -- Options for HTML output ---------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = "ampl_sphinx_theme"
html_theme_options = {
    "icon_links": [
        {
            "name": "GitHub",
            "url": "https://github.com/ampl/plugins",
            "icon": "fab fa-github",
        },
    ],
    "logo_text": "Plugins",
}
html_context = {"default_mode": "light"}
googleanalytics_id = "G-4MQXTJNBKD"

html_static_path = ["_static"]
# html_css_files = [
#     'css/custom.css',
# ]
# The name for this set of Sphinx documents.  If None, it defaults to
# "<project> v<release> documentation".
html_title = "AMPL Plugins documentation"

html_baseurl = "https://plugins.ampl.com/"

html_extra_path = ["_html"]

sitemap_filename = "sphinx-sitemap.xml"

# A shorter title for the navigation bar.  Default is the same as html_title.
html_short_title = "AMPL Plugins"

# The name of an image file (relative to this directory) to place at the top
# of the sidebar.
html_logo = "https://raw.githubusercontent.com/ampl/ampl.github.io/master/themes/static/ampl-navbar-logo.png"

# Add favicon
html_favicon = "https://raw.githubusercontent.com/ampl/ampl.github.io/master/themes/static/ampl-favicon.png"

# If true, links to the reST sources are added to the pages.
html_show_sourcelink = False

# If true, "Created using Sphinx" is shown in the HTML footer. Default is True.
html_show_sphinx = False


# Output file base name for HTML help builder.
htmlhelp_basename = "AMPLdoc"


# -- Options for LaTeX output --------------------------------------------------

latex_elements = {
    # The paper size ('letterpaper' or 'a4paper').
    # 'papersize': 'letterpaper',
    # The font size ('10pt', '11pt' or '12pt').
    # 'pointsize': '10pt',
    # Additional stuff for the LaTeX preamble.
    # 'preamble': '',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title, author, documentclass [howto/manual]).
latex_documents = [
    (
        "index",
        "amplplugins.tex",
        "AMPL Plugins Documentation",
        "AMPL Optimization Inc.",
        "manual",
    ),
]

# The name of an image file (relative to this directory) to place at the top of
# the title page.
# latex_logo = None

# For "manual" documents, if this is true, then toplevel headings are parts,
# not chapters.
# latex_use_parts = False

# If true, show page references after internal links.
# latex_show_pagerefs = False

# If true, show URL addresses after external links.
# latex_show_urls = False

# Documents to append as an appendix to all manuals.
# latex_appendices = []

# If false, no module index is generated.
# latex_domain_indices = True


# -- Options for manual page output --------------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    (
        "index",
        "amplplugins",
        "AMPL Plugins Documentation",
        ["AMPL Optimization Inc."],
        1,
    )
]

# If true, show URL addresses after external links.
# man_show_urls = False


# -- Options for Texinfo output ------------------------------------------------

# Grouping the document tree into Texinfo files. List of tuples
# (source start file, target name, title, author,
#  dir menu entry, description, category)
texinfo_documents = [
    (
        "index",
        "amplplugins",
        "AMPL Plugins Documentation",
        "AMPL Optimization Inc.",
        "AMPL",
        "Documentation for AMPL plugins.",
        "Miscellaneous",
    ),
]

# Documents to append as an appendix to all manuals.
# texinfo_appendices = []

# If false, no module index is generated.
# texinfo_domain_indices = True

# How to display URL addresses: 'footnote', 'no', or 'inline'.
# texinfo_show_urls = 'footnote'


def run_doxygen(folder):
    """Run the doxygen make command in the designated folder"""

    try:
        print("cd %s; doxygen" % folder)
        retcode = subprocess.call("doxygen", cwd=folder, shell=True)
        if retcode < 0:
            sys.stderr.write("doxygen terminated by signal %s" % (-retcode))
    except OSError as e:
        sys.stderr.write("doxygen execution failed: %s" % e)


def generate_doxygen_xml(app):
    """Run the doxygen make commands if we're on the ReadTheDocs server"""
    # read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'
    # We always build doxygen documentation
    run_doxygen(os.path.dirname(__file__) or os.curdir)


def setup(app):
    # Add hook for building doxygen xml
    app.connect("builder-inited", generate_doxygen_xml)
