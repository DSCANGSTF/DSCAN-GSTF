class Team:

    def __init__(self):
        self.members = set()
        self.member_skills = dict()  # dictionary of list of skills representing by an member to the Team
        self.skillWeights = dict()
	self.location = (,)
        self.task = set()
        self.random_members = set()

    def get_detailed_record(self):
        pass

    def is_formed(self):
        all_skills = set()
        for member in self.member_skills:
            all_skills.update(set(self.member_skills[member]))
        return len(all_skills) == len(self.task)

    def __str__(self):  # real signature unknown
        if len(self.members) == 0:
            return "Team Not Yet Formed"
        else:
            return ":".join(self.members)

    def clean_it(self):
        self.members = set()
        self.member_skills = dict()  # dictionary of list of skills representing by an member to the Team
        self.skillWeights  = dict()
        self.task = set()
        self.random_members = set()

    def cardinality(self):
        return len(self.members)

    def diameter(self, l_graph) -> float:
        """
        return diameter of graph formed by team
        diam(X) := max{sp_{X}(u,v) | u,v ∈ X}.
        :param l_graph:
        :return:
        """
        import networkx as nx
        t_graph = self.get_team_graph(l_graph)
        if nx.number_of_nodes(t_graph) < 2:
            return 0
        else:
            sp = dict()
            for nd in t_graph.nodes:
                sp[nd] = nx.single_source_dijkstra_path_length(t_graph, nd)
            e = nx.eccentricity(t_graph, sp=sp)
            return round(nx.diameter(t_graph, e), 5)

    def radius(self, l_graph) -> float:
        """
        return diameter of graph formed by team
        :param l_graph:
        :return:
        """
        import networkx as nx
        import matplotlib.pyplot as plt
        t_graph = self.get_team_graph(l_graph)
        if nx.number_of_nodes(t_graph) < 2:
            return 0
        else:
            shp = dict()
            for nd in t_graph.nodes:
                shp[nd] = nx.single_source_dijkstra_path_length(t_graph, nd)
            try:
                eccent = nx.eccentricity(t_graph, sp=shp)
            except TypeError as eccent:
                nx.draw_circular(t_graph, with_labels=True)
                plt.show()
                msg = "Found infinite path length because the graph is not" " connected"
                raise nx.NetworkXError(msg) from eccent
            return round(nx.radius(t_graph, eccent), 5)

   

    def random-member_distance(self, l_graph) -> float:
        """
        return member distance of team from task location
        :param l_graph:
        :return:
        """
        import networkx as nx
        ld = 0
        if len(self.members) < 2:
            return 0
        else:
            for member in self.members:
                if member != self.leader:
                    if nx.has_path(l_graph, self.leader, member):
                        ld += nx.dijkstra_path_length(l_graph, self.leader, member, weight="weight")
        return round(ld, 3)

   


if __name__ == "__main__":
    
    import networkx as nx

    from Team import Team

    team = Team()
    print(team.get_diameter_nodes(graph))
    # print("memory required in bytes : " + str(team.__sizeof__()))  # sizeof
    # print("memory required in bytes with overhead : " + str(sys.getsizeof(team)))  # sizeof with overhead
    # print("string " + team.__str__())
    # print("cardinality " + str(team.cardinality()))
