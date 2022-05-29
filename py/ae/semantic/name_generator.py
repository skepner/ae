import ae_backend

# ======================================================================

class NameGenerator:

    def __init__(self):
        self.locdb = ae_backend.locdb_v3.locdb()

    def location_isolation_year2_passaga_type(self, antigen):
        if name_parsing_result := ae_backend.virus.name_parse(antigen.name()):
            name = f"{self.locdb.abbreviation(name_parsing_result.parts.location)}/{name_parsing_result.parts.isolation}/{name_parsing_result.parts.year[2:]}"
        else:
            name = antigen.name()
        return f"{name}-{self.passage_type(antigen)}"

    def location_year2_passaga_type(self, antigen):
        if name_parsing_result := ae_backend.virus.name_parse(antigen.name()):
            name = f"{self.locdb.abbreviation(name_parsing_result.parts.location)}/{name_parsing_result.parts.year[2:]}"
        else:
            name = antigen.name()
        return f"{name}-{self.passage_type(antigen)}"

    def passage_type(self, antigen):
        if reassortant := antigen.reassortant():
            return f"{reassortant}-egg"
        else:
            return antigen.passage().passage_type()
